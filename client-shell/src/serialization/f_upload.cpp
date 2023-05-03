#include <utility>
#include <io/file.h>

#include "f_upload.h"
#include "protocol/messages.h"
#include "util/sha512.h"

namespace uh::client::serialization
{

// ---------------------------------------------------------------------

f_upload::f_upload(protocol::client_pool& cl_pool,
                   uhv::job_queue<std::unique_ptr<uhv::f_meta_data>>& in_jq,
                   uhv::job_queue<std::unique_ptr<uhv::f_meta_data>>& out_jq,
                   uh::client::chunking::mod& chunking,
                   std::filesystem::path uhv_path,
                   unsigned int num_threads)
    : common::thread_manager(num_threads),
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


protocol::block_meta_data f_upload::send_xs_blocks (auto &client_handle, auto &xsmall_blocks_req) {
    protocol::block_meta_data meta_data;
    auto res = client_handle->write_xsmall_blocks(xsmall_blocks_req);
    protocol::write_xsmall_blocks::request new_req;
    std::swap (xsmall_blocks_req, new_req);
    meta_data.hash.insert(meta_data.hash.end(), res.hashes.begin(), res.hashes.end());
    meta_data.effective_size = res.effective_size;
    return meta_data;
}

void f_upload::send_statistics()
{
    uh::protocol::blob uhv_path {};
    std::ranges::copy(m_uhv_path.string(), std::back_inserter(uhv_path));

    const uh::protocol::blob uhv_id {uh::util::sha512(uhv_path)};

    uh::protocol::client_statistics::request client_stat {
            uhv_id, m_uploaded_size };

    protocol::client_pool::handle client_handle = m_client_pool.get();
    client_handle->send_client_statistics(client_stat);
}

// ---------------------------------------------------------------------

void f_upload::chunk_and_upload(std::unique_ptr<uhv::f_meta_data>& f_meta_data,
                                protocol::client_pool::handle& client_handle)
{
    if ( f_meta_data->f_type() == uhv::uh_file_type::regular )
    {
        io::file file(f_meta_data->f_path());

        uh::chunking::buffer buffer (file, uh::protocol::server::MAXIMUM_BLOCK_SIZE);
        auto chunker = m_chunking.create_chunker(buffer);
        std::vector <uint32_t> chunk_sizes;

        for (auto chunk = chunker->next_chunk(); !chunk.empty(); chunk = chunker->next_chunk()) {
            chunk_sizes.push_back(chunk.size());
            if (buffer.length() == 0) {
                protocol::write_chunks::response resp = client_handle->write_chunks({chunk_sizes, buffer.data()});
                f_meta_data->add_hash(resp.hashes);
                f_meta_data->add_effective_size(resp.effective_size);
                chunk_sizes.clear();
            }
        }


        m_uploaded_size += f_meta_data->f_size();


    }

    m_output_jq.append_job(std::move(f_meta_data));
}

// ---------------------------------------------------------------------

void f_upload::spawn_threads()
{
    for (size_t i = 0; i < m_num_threads; i++)
    {
        m_thread_pool.emplace_back([&]()
        {
           protocol::client_pool::handle&& client_connection_handle = m_client_pool.get();

           while (auto job = m_input_jq.get_job())
           {
                if (job == std::nullopt)
                {
                   break;
                }

                auto filename = (*job)->f_path();

                try
                {
                    chunk_and_upload(*job, client_connection_handle);
                    add_result(filename);
                }
                catch (const std::exception& e)
                {
                    add_result(filename, e.what());
                }
           }
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

} // namespace uh::client::serialization
