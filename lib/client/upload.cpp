#include "upload.h"

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
    file_handle(std::unique_ptr<uhv::meta_data>&& md)
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

    uhv::meta_data& metadata()
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
    std::promise<std::unique_ptr<uhv::meta_data>> m_promise;
    std::unique_ptr<uhv::meta_data> m_metadata;

    unsigned m_chunks_missing = 0;
    bool m_queued = false;
};

// ---------------------------------------------------------------------

class request
{
public:
    request()
        : m_buffer(reinterpret_cast<char*>(malloc(upload::MAXIMUM_DATA_SIZE))),
          m_size(upload::MAXIMUM_DATA_SIZE)
    {
        if (m_buffer == 0)
        {
            throw std::bad_alloc();
        }
    }

    ~request()
    {
        free(m_buffer);
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
        return m_size - m_offs;
    }

    void send(protocol::client_pool::handle& client)
    {
        if (m_files.empty())
        {
            return;
        }

        auto resp = client->write_chunks(write_chunks::request{m_chunk_sizes, std::span(m_buffer, m_offs)});

        auto hash_size = resp.hashes.size() / m_files.size();
        ASSERT(resp.hashes.size() % m_files.size() == 0);

        for (auto index = 0u; index < m_files.size();)
        {
            auto it = std::find_if(m_files.begin() + index, m_files.end(),
                [&, this](auto f){ return f != m_files[index]; });

            auto count = std::distance(m_files.begin() + index, it);

            file_handle& fh = *m_files[index];
            uhv::meta_data& md = fh.metadata();

            md.append_hashes(resp.hashes.begin() + index * hash_size,
                             resp.hashes.begin() + (index + count) * hash_size);
            md.append_sizes(m_chunk_sizes.begin() + index,
                            m_chunk_sizes.begin() + index + count);

            // TODO: send effective size per chunk meta_data->add_effective_size(resp.effective_size);

            fh.finished(count);
            index += count;
        }
    }

    std::span<char> buffer()
    {
        return std::span<char>(m_buffer + m_offs, m_buffer + m_size);
    }

    void add_chunk(file_handle* fh, std::size_t size)
    {
        m_files.push_back(fh);
        m_chunk_sizes.push_back(size);
        m_offs += size;
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
    char* m_buffer;
    std::size_t m_size;
    std::size_t m_offs = 0;
};

// ---------------------------------------------------------------------

class buffers
{
public:

    buffers(protocol::client_pool::handle&& client_handle, std::size_t queue_size = 4)
        : client_handle(std::move(client_handle)),
          m_requests(queue_size),
          m_mtx(),
          m_active(0),
          m_active_cond(),
          m_send(0),
          m_send_cond(),
          m_running(true),
          m_thread([this](){ worker(); })
    {
    }

    request& active()
    {
        return m_requests[m_active];
    }

    /**
     * Queue the current buffer for writing, move to the next available buffer, possibly
     * waiting for one becoming available.
     */
    void next()
    {
        std::unique_lock lk(m_mtx);
        auto next = (m_active + 1) % m_requests.size();

        if (next == m_send)
        {
            m_send_cond.wait(lk, [&, this](){ return next != m_send; });
        }

        m_requests[next].reset();
        m_active = next;
        m_active_cond.notify_all();
    }

    /**
     * Wait for a buffer becoming ready to send. Send this buffer
     */
    void worker()
    {
        while (m_running)
        {
            std::unique_lock lk(m_mtx);
            if (m_send == m_active)
            {
                m_active_cond.wait(lk, [this](){ return m_send != m_active || !m_running; });
            }

            while (m_send != m_active)
            {
                lk.unlock();
                m_requests[m_send].send(client_handle);
                lk.lock();
                m_send = (m_send + 1) % m_requests.size();
                m_send_cond.notify_all();
            }
        }
    }

    void stop()
    {
        {
            std::unique_lock lk(m_mtx);
            if (m_active != m_send)
            {
                m_send_cond.wait(lk, [&, this](){ return m_active == m_send; });
            }

            m_running = false;
            m_active_cond.notify_all();
        }

        m_thread.join();

        m_requests[m_active].send(client_handle);
    }

private:
    protocol::client_pool::handle client_handle;
    std::vector<request> m_requests;

    std::mutex m_mtx;

    std::atomic<std::size_t> m_active;
    std::condition_variable m_active_cond;

    std::atomic<std::size_t> m_send;
    std::condition_variable m_send_cond;

    bool m_running;
    std::thread m_thread;
};

// ---------------------------------------------------------------------

upload::upload(protocol::client_pool& cl_pool,
               uhv::job_queue<std::unique_ptr<uhv::meta_data>>& in_jq,
               std::list<std::future<std::unique_ptr<uhv::meta_data>>>& out_jq,
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

upload::~upload()
{
    join();
    send_statistics();
}

// ---------------------------------------------------------------------

void upload::join()
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

void upload::send_statistics()
{
    uh::protocol::blob uhv_path {};
    std::ranges::copy(m_uhv_path.string(), std::back_inserter(uhv_path));

    uh::protocol::client_statistics::request client_stat {
            uhv_path, m_uploaded_size };

    protocol::client_pool::handle client_handle = m_client_pool.get();
    client_handle->send_client_statistics(client_stat);
}

// ---------------------------------------------------------------------

void upload::chunk_and_upload(std::unique_ptr<uhv::meta_data>&& md_ptr, buffers& r)
{
    if (md_ptr->type() == uhv::uh_file_type::regular && md_ptr->size() != 0)
    {
        auto& md = *md_ptr;
        auto fh = std::make_shared<file_handle>(std::move(md_ptr));
        r.active().add_handle(fh);
        m_output_jq.push_back(fh->get_future());

        io::file file(md.path());
        auto chunker = m_chunking.create_chunker(file, md.size());

        bool busy = true;
        while (busy)
        {
            auto res = chunker->chunk(r.active().buffer());
            switch (res.type)
            {
                case chunking::chunk_result::done:
                    busy = false;
                    break;

                case chunking::chunk_result::too_small:
                    r.next();
                    r.active().add_handle(fh);
                    break;

                case chunking::chunk_result::created:
                    r.active().add_chunk(fh.get(), res.size);
                    break;
            }
        }

        m_uploaded_size += md.size();
        fh->ready();
    }
    else
    {
        std::promise<std::unique_ptr<uhv::meta_data>> promise;
        m_output_jq.push_back(promise.get_future());
        promise.set_value(std::move(md_ptr));
    }
}

// ---------------------------------------------------------------------

void upload::spawn_threads()
{
    for (size_t i = 0; i < m_num_threads; i++)
    {
        m_thread_pool.emplace_back([&]()
        {
            try
            {
                buffers db(m_client_pool.get());

                while (auto job = m_input_jq.get_job())
                {
                    if (job == std::nullopt)
                    {
                        break;
                    }

                    auto filename = (*job)->path();

                    try
                    {
                        chunk_and_upload(std::move(*job), db);
                        add_result(filename);
                    }
                    catch (const std::exception& e)
                    {
                        add_result(filename, e.what());
                    }
                }

                db.stop();
            }
            catch (const std::exception& e)
            {
                std::cerr << "Error: " << e.what() << "\n";
            }
        });
    }
}

// ---------------------------------------------------------------------

const std::map<std::filesystem::path, std::optional<std::string>>& upload::results() const
{
    return m_results;
}

// ---------------------------------------------------------------------

void upload::add_result(const std::filesystem::path& p,
                          const std::optional<std::string>& error)
{
    const std::lock_guard<std::mutex> lock(m_result_mutex);
    m_results[p] = error;
}

// ---------------------------------------------------------------------

} // namespace uh::client
