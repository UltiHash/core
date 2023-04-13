#include "f_download.h"

namespace uh::client::serialization
{

// ---------------------------------------------------------------------

f_download::f_download(protocol::client_pool& cl_pool,
                       uhv::job_queue<std::unique_ptr<uhv::f_meta_data>>& jq,
                       std::filesystem::path dest_path,
                       unsigned int num_threads)
    : common::thread_manager(num_threads),
      m_input_jq(jq),
      m_client_pool(cl_pool),
      m_dest_path(std::move(dest_path))
{
}

// ---------------------------------------------------------------------

f_download::~f_download()
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

void f_download::download_file(std::unique_ptr<uhv::f_meta_data>& f_meta_data)
{
    if (f_meta_data->f_type() == uhv::uh_file_type::regular)
    {
        std::ofstream new_file(f_meta_data->f_path(),
                                 std::ios::app | std::ios::binary);

        if (!new_file.is_open())
            throw std::runtime_error("Error: Could not open file "
                                     + f_meta_data->f_path().string() + "\n");

        std::vector<char> buffer(64);

        auto client = m_client_pool.get();
        for (auto i = 0; i < f_meta_data->f_hashes().size(); i += 64)
        {
            std::copy(f_meta_data->f_hashes().begin() + i,
                      f_meta_data->f_hashes().begin() + i + 64, buffer.begin());

            new_file << *client->read_block(buffer);
        }

        new_file.flush();
        new_file.close();
    }
    else if (f_meta_data->f_type() == uhv::uh_file_type::none)
    {
        throw std::runtime_error("Unknown file type encountered: " + f_meta_data->f_path().string());
    }

    std::filesystem::permissions(f_meta_data->f_path(),
                                 static_cast<std::filesystem::perms>(f_meta_data->f_permissions()),
                                 std::filesystem::perm_options::replace);
}

// ---------------------------------------------------------------------

void f_download::spawn_threads()
{
    for (size_t i = 0; i < m_num_threads; i++)
    {
        m_thread_pool.emplace_back([&]()
        {
            while (auto item = m_input_jq.get_job())
            {
                if (item == std::nullopt)
                {
                    break;
                }

                try
                {
                    download_file(*item);
                    add_result((*item)->f_path());
                }
                catch (const std::exception& e)
                {
                    add_result((*item)->f_path(), e.what());
                }
            }
        });
    }
}

// ---------------------------------------------------------------------

const std::map<std::filesystem::path, std::optional<std::string>>& f_download::results() const
{
    return m_results;
}

// ---------------------------------------------------------------------

void f_download::add_result(const std::filesystem::path& p,
                            const std::optional<std::string>& error)
{
    const std::lock_guard<std::mutex> lock(m_result_mutex);
    m_results[p] = error;
}

// ---------------------------------------------------------------------

} // namespace uh::client::serialization
