#include "f_download.h"

#include <util/exception.h>
#include <protocol/server.h>


namespace uh::client
{

// ---------------------------------------------------------------------

f_download::f_download(protocol::client_pool& cl_pool,
                       uhv::job_queue<std::unique_ptr<uhv::f_meta_data>>& jq,
                       std::filesystem::path dest_path,
                       unsigned int num_threads)
    : thread_manager(num_threads),
      m_input_jq(jq),
      m_client_pool(cl_pool),
      m_dest_path(std::move(dest_path)),
      m_size(0)
{
}

// ---------------------------------------------------------------------

f_download::~f_download()
{
    join();
}

// ---------------------------------------------------------------------

void f_download::join()
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

void f_download::download_file(std::unique_ptr<uhv::f_meta_data>& f_meta_data)
{
    auto path = m_dest_path / f_meta_data->f_path();
    std::filesystem::create_directories(path.parent_path());

    if (f_meta_data->f_type() == uhv::uh_file_type::regular)
    {
        std::ofstream new_file(path, std::ios::binary | std::ios::trunc);
        if (!new_file.is_open())
        {
            THROW(util::exception, "Error: Could not open file " + path.string());
        }

        auto client = m_client_pool.get();

        size_t aggregated_size = 0;
        size_t offset = 0;

        // here we assume that each chunk size is smaller than MAXIMUM_DATA_SIZE
        for (size_t i = 0; i < f_meta_data->f_chunk_sizes().size(); ++i)
        {
            aggregated_size += f_meta_data->f_chunk_sizes()[i];
            if (aggregated_size > protocol::server::MAXIMUM_DATA_SIZE)
            {
                auto resp = client->read_chunks (
                        {{const_cast <char *> (f_meta_data->f_hashes().data() + offset), (i - 1) * 64}});
                const auto& data = std::get<0>(resp.data);
                new_file.write (data.data(), data.size());
                offset = (i - 1) * 64;
                aggregated_size = f_meta_data->f_chunk_sizes()[i];
            }
        }

        if (aggregated_size > 0)
        {
            auto resp = client->read_chunks ({{const_cast <char *> (f_meta_data->f_hashes().data() + offset),
                                               f_meta_data->f_hashes().size() - offset}});

            const auto& data = std::get<0>(resp.data);
            m_size += data.size();
            new_file.write(data.data(), data.size());
        }

        std::filesystem::permissions(path,
                                    static_cast<std::filesystem::perms>(f_meta_data->f_permissions()),
                                    std::filesystem::perm_options::replace);
    }
    else if (f_meta_data->f_type() == uhv::uh_file_type::directory)
    {
        std::filesystem::create_directories(path);
        std::filesystem::permissions(path,
                                    static_cast<std::filesystem::perms>(f_meta_data->f_permissions()),
                                    std::filesystem::perm_options::replace);
    }

    else if (f_meta_data->f_type() == uhv::uh_file_type::none)
    {
        throw std::runtime_error("Unknown file type encountered: " + f_meta_data->f_path().string());
    }

    std::filesystem::permissions(path,
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

std::size_t f_download::size() const
{
    return m_size;
}

// ---------------------------------------------------------------------

void f_download::add_result(const std::filesystem::path& p,
                            const std::optional<std::string>& error)
{
    const std::lock_guard<std::mutex> lock(m_result_mutex);
    m_results[p] = error;
}

// ---------------------------------------------------------------------

} // namespace uh::client
