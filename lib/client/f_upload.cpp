#include "f_upload.h"

#include <io/file.h>
#include <protocol/messages.h>

#include <algorithm>
#include <set>
#include <utility>

#include <iostream>


using namespace uh::protocol;

namespace uh::client
{

// ---------------------------------------------------------------------

class file_handle
{
public:
    file_handle(std::unique_ptr<uhv::f_meta_data>&& md)
        : m_metadata(std::move(md))
    {
    }

    void finished(unsigned count)
    {
        m_chunks_missing -= count;
        if (m_chunks_missing == 0 && m_queued)
        {
            m_promise.set_value(std::move(m_metadata));
        }
    }

    uhv::f_meta_data& metadata()
    {
        return *m_metadata;
    }

    void add_chunk()
    {
        m_chunks_missing++;
    }

    auto get_future()
    {
        return m_promise.get_future();
    }

    void ready()
    {
        m_queued = true;
    }
private:
    std::promise<std::unique_ptr<uhv::f_meta_data>> m_promise;
    std::unique_ptr<uhv::f_meta_data> m_metadata;

    unsigned m_chunks_missing = 0;
    bool m_queued = false;
};

// ---------------------------------------------------------------------

class request
{
public:
    request()
        : m_buffer(f_upload::MAXIMUM_DATA_SIZE)
    {
    }

    void reset()
    {
        m_offs = 0;
        m_files.clear();
        m_chunk_sizes.clear();
        m_handles.clear();
    }

    std::size_t space_left() const
    {
        return m_buffer.size() - m_offs;
    }

    void send(protocol::client_pool::handle& client)
    {
        auto resp = client->write_chunks(write_chunks::request{m_chunk_sizes, std::span(m_buffer.data(), m_offs)});

        auto hash_size = resp.hashes.size() / m_files.size();
        ASSERT(resp.hashes.size() % m_files.size() == 0);

        for (auto index = 0u; index < m_files.size();)
        {
            auto it = std::find_if(m_files.begin() + index, m_files.end(),
                [&, this](auto f){ return f != m_files[index]; });

            auto count = std::distance(m_files.begin() + index, it);

            file_handle& fh = *m_files[index];
            uhv::f_meta_data& md = fh.metadata();

            md.append_hashes(resp.hashes.begin() + index * hash_size,
                             resp.hashes.begin() + (index + count) * hash_size);
            md.append_sizes(m_chunk_sizes.begin() + index,
                            m_chunk_sizes.begin() + index + count);

            // TODO: send effective size per chunk f_meta_data->add_effective_size(resp.effective_size);

            fh.finished(count);
            index += count;
        }
    }

    void add_chunk(file_handle* fh, std::span<char> chunk)
    {
        m_files.push_back(fh);
        m_chunk_sizes.push_back(chunk.size());
        std::memcpy(&m_buffer[m_offs], chunk.data(), chunk.size());
        m_offs += chunk.size();
        fh->add_chunk();
    }

    void add_handle(std::shared_ptr<file_handle> fh)
    {
        m_handles.insert(fh);
    }

private:
    std::set<std::shared_ptr<file_handle>> m_handles;
    std::vector<file_handle*> m_files;
    std::vector<uint32_t> m_chunk_sizes;
    std::vector<char> m_buffer;
    std::size_t m_offs = 0;
};

// ---------------------------------------------------------------------

f_upload::f_upload(protocol::client_pool& cl_pool,
                   uhv::job_queue<std::unique_ptr<uhv::f_meta_data>>& in_jq,
                   std::list<std::future<std::unique_ptr<uhv::f_meta_data>>>& out_jq,
                   uh::chunking::mod& chunking,
                   std::filesystem::path uhv_path,
                   unsigned int num_threads)
    : thread_manager(num_threads),
      m_input_jq(in_jq),
      m_output_jq(out_jq),
      m_client_pool(cl_pool),
      m_chunking(chunking),
      m_uhv_path(std::move(uhv_path))
{
}

// ---------------------------------------------------------------------

f_upload::~f_upload()
{
    join();
    send_statistics();
}

// ---------------------------------------------------------------------

void f_upload::join()
{
    m_input_jq.stop();

    for (auto& thread : m_thread_pool)
    {
        if (thread.joinable())
        {
            thread.join();
        }
    }

    m_thread_pool.clear();
}

// ---------------------------------------------------------------------

void f_upload::send_statistics()
{
    uh::protocol::blob uhv_path {};
    std::ranges::copy(m_uhv_path.string(), std::back_inserter(uhv_path));

    uh::protocol::client_statistics::request client_stat {
            uhv_path, m_uploaded_size };

    protocol::client_pool::handle client_handle = m_client_pool.get();
    client_handle->send_client_statistics(client_stat);
}

// ---------------------------------------------------------------------

void f_upload::chunk_and_upload(std::unique_ptr<uhv::f_meta_data>&& md_ptr,
                                protocol::client_pool::handle& client_handle,
                                request& r)
{
    if (md_ptr->f_type() == uhv::uh_file_type::regular && md_ptr->f_size() != 0)
    {
        auto& md = *md_ptr;
        auto fh = std::make_shared<file_handle>(std::move(md_ptr));
        r.add_handle(fh);
        m_output_jq.push_back(fh->get_future());

        io::file file(md.f_path());

        auto chunker = m_chunking.create_chunker(file,  std::min (uh::protocol::server::MAXIMUM_DATA_SIZE, static_cast <const size_t> (md.f_size())));

        for (auto chunk = chunker->next_chunk(); !chunk.empty(); chunk = chunker->next_chunk())
        {
            if (r.space_left() < chunk.size())
            {
                r.send(client_handle);
                r.reset();
                r.add_handle(fh);
            }

            r.add_chunk(fh.get(), chunk);
        }

        m_uploaded_size += md.f_size();

        fh->ready();
    }
    else
    {
        std::promise<std::unique_ptr<uhv::f_meta_data>> promise;
        m_output_jq.push_back(promise.get_future());
        promise.set_value(std::move(md_ptr));
    }
}

// ---------------------------------------------------------------------

void f_upload::spawn_threads()
{
    for (size_t i = 0; i < m_num_threads; i++)
    {
        m_thread_pool.emplace_back([&]()
        {
           protocol::client_pool::handle&& client_connection_handle = m_client_pool.get();
           request req;

           while (auto job = m_input_jq.get_job())
           {
                if (job == std::nullopt)
                {
                   break;
                }

                auto filename = (*job)->f_path();

                try
                {
                    chunk_and_upload(std::move(*job), client_connection_handle, req);
                    add_result(filename);
                }
                catch (const std::exception& e)
                {
                    add_result(filename, e.what());
                }
           }

           req.send(client_connection_handle);
        });
    }
}

// ---------------------------------------------------------------------

const std::map<std::filesystem::path, std::optional<std::string>>& f_upload::results() const
{
    return m_results;
}

// ---------------------------------------------------------------------

void f_upload::add_result(const std::filesystem::path& p,
                          const std::optional<std::string>& error)
{
    const std::lock_guard<std::mutex> lock(m_result_mutex);
    m_results[p] = error;
}

// ---------------------------------------------------------------------

} // namespace uh::client
