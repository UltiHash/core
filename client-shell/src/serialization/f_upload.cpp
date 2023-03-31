#include <fstream>
#include "f_upload.h"

namespace uh::client::serialization
{

// ---------------------------------------------------------------------

f_upload::f_upload(std::unique_ptr<protocol::client_pool>& cl_pool,
                   common::job_queue<std::unique_ptr<common::f_meta_data>>& in_jq,
                   common::job_queue<std::unique_ptr<common::f_meta_data>>& out_jq,
                   uh::client::chunking::file_chunker& chunker,
                   unsigned int num_threads):
                   m_client_pool(cl_pool),
                   m_input_jq(in_jq),
                   m_output_jq(out_jq),
                   m_chunker(chunker),
                   common::thread_manager(num_threads)
{
}

// ---------------------------------------------------------------------

f_upload::~f_upload()
{
    m_input_jq.stop();
    for (auto& thread : m_thread_pool)
    {
        INFO << "Joining Thread ";
        if (thread.joinable())
            thread.join();
    }
}

// ---------------------------------------------------------------------

void f_upload::chunk_and_upload(std::unique_ptr<common::f_meta_data>& f_meta_data,
                                protocol::client_pool::handle& client_handle)
{
    if ( f_meta_data->f_type() == common::uh_file_type::regular )
    {
        auto chunks = m_chunker.chunk_files(f_meta_data);

        for (auto & chunk : chunks)
        {
            auto alloc = client_handle->allocate(chunk.size());
            io::write_from_buffer(alloc->device(), chunk);

            auto meta_data = alloc->persist();
            f_meta_data->add_hash(meta_data.hash);
            f_meta_data->add_effective_size(meta_data.effective_size);
        }
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
               protocol::client_pool::handle&& client_connection_handle = m_client_pool->get();

               while (auto job = m_input_jq.get_job())
               {
                    if (job == std::nullopt)
                    {
                       break;
                    }
                    else
                    {
                        chunk_and_upload(job.value(),
                            client_connection_handle);
                    }
               }
        });
    }
}

// ---------------------------------------------------------------------

} // namespace uh::client::serialization
