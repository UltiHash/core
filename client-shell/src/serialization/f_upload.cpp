#include <fstream>
#include "f_upload.h"

namespace uh::client::serialization
{

// ---------------------------------------------------------------------

f_upload::f_upload(std::unique_ptr<protocol::client_pool>& cl_pool,
                   common::job_queue<std::unique_ptr<common::f_meta_data>>& in_jq,
                   common::job_queue<std::unique_ptr<common::f_meta_data>>& out_jq,
                   unsigned int num_threads) :
                   m_client_pool(cl_pool),
                   m_input_jq(in_jq),
                   m_output_jq(out_jq),
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

void f_upload::upload_files(std::unique_ptr<common::f_meta_data>& f_meta_data,
                            protocol::client_pool::handle& client_handle)
{

    if ( f_meta_data->f_type() == common::uh_file_type::regular )
    {
        std::ifstream input_file(f_meta_data->f_path(),
                                 std::ios::in | std::ios::binary);

        if (!input_file.is_open())
            throw std::ios_base::failure("Error: Could not open file "
            + f_meta_data->f_path().string() + "\n");

        if (input_file.peek() != std::ifstream::traits_type::eof())
        {

            constexpr std::uint64_t buf_size = 1 << 22;
            std::vector<char> tmp_buffer;
            tmp_buffer.reserve(std::min<std::uint64_t>(f_meta_data->f_size(), buf_size));

            while (input_file)
            {
                auto remaining_size = f_meta_data->f_size() - input_file.tellg();
                tmp_buffer.resize(std::min<std::size_t>(remaining_size, buf_size));

                input_file.read((tmp_buffer.data()), tmp_buffer.size());
                std::streamsize bytes_read = input_file.gcount();

                if (input_file.fail() || input_file.bad())
                {
                    throw std::ios_base::failure("Error reading from file");
                }
                else if (bytes_read == 0)
                {
                    break;
                }

                auto alloc = client_handle->allocate(tmp_buffer.size());
                io::write_from_buffer(alloc->device(), tmp_buffer);

                auto meta_data = alloc->persist();
                f_meta_data->add_hash(meta_data.hash);
                f_meta_data->add_effective_size(meta_data.effective_size);
            }

        }

        input_file.close();
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

               while (auto&& item = m_input_jq.get_job())
               {
                   if (item == std::nullopt)
                       break;
                   else
                       upload_files(item.value(),
                                    client_connection_handle);

               }

           });
    }
}

// ---------------------------------------------------------------------

} // namespace uh::client::serialization
