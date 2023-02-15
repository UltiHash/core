#include <fstream>
#include "f_upload.h"

namespace uh::client::serialization
{

// ---------------------------------------------------------------------

f_upload::f_upload(std::unique_ptr<protocol::client_pool>&& cl_pool,
                   common::job_queue<std::unique_ptr<common::f_meta_data>>& in_jq,
                   common::job_queue<std::unique_ptr<common::f_meta_data>>& out_jq,
                   size_t num_threads) :
        m_client_pool(std::move(cl_pool)), m_input_jq(in_jq), m_output_jq(out_jq), common::thread_manager(num_threads)
{

}

// ---------------------------------------------------------------------

f_upload::~f_upload()
{
    m_input_jq.stop();
}

// ---------------------------------------------------------------------

void f_upload::upload_files(std::unique_ptr<common::f_meta_data>&& f_meta_data,
                            protocol::client_pool::handle& client_handle)
{
    if (S_ISREG(f_meta_data->get_f_stat().st_mode))
    {

        std::ifstream input_file(f_meta_data->get_f_path(),std::ios::in | std::ios::binary);

        if (!input_file.is_open())
            throw std::runtime_error("Error: Could not open file " + f_meta_data->get_f_path().string() + "\n");

        if (input_file.peek() != std::ifstream::traits_type::eof())
        {
            constexpr std::size_t buf_size = 1 << 22;
            std::vector<char> tmp_buffer(std::min(std::size_t(input_file.tellg()), buf_size));

            while (true)
            {
                input_file.read((tmp_buffer.data()), buf_size);
                std::streamsize bytes_read = input_file.gcount();

                if (bytes_read == 0)
                {
                    break;
                }

                auto recv_hash = client_handle.m_client->write_chunk(tmp_buffer);
                f_meta_data->add_hash(recv_hash);

            }

        }

        input_file.close();
    }

    m_output_jq.put_back_job(std::move(f_meta_data));
}

// ---------------------------------------------------------------------

void f_upload::spawn_threads()
{
    for (size_t i = 0; i < m_num_threads; i++)
    {
        m_thread_pool.emplace_back([&]()
           {

               protocol::client_pool::handle&& client_connection_handle = m_client_pool->get();

               while (auto&& item = m_input_jq.get_job())
               {

                   if (item == std::nullopt)
                       break;
                   else
                       upload_files(std::move(item.value()), client_connection_handle);

               }

               client_connection_handle.m_pool.put_back(std::move(client_connection_handle.m_client));

           });
    }
}

// ---------------------------------------------------------------------

} // namespace uh::client::serialization
