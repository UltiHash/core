#include "download.h"

#include <util/exception.h>
#include <protocol/server.h>


namespace uh::client
{

// ---------------------------------------------------------------------

download::download(protocol::client_pool& cl_pool,
                   uhv::job_queue<std::unique_ptr<uhv::meta_data>>& jq,
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

download::~download()
{
    join();
}

// ---------------------------------------------------------------------

void download::join()
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

void download::download_file(std::unique_ptr<uhv::meta_data>& meta_data)
{
    auto path = m_dest_path / meta_data->path();
    std::filesystem::create_directories(path.parent_path());

    if (meta_data->type() == uhv::uh_file_type::regular)
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
        for (size_t i = 0; i < meta_data->chunk_sizes().size(); ++i)
        {
            aggregated_size += meta_data->chunk_sizes()[i];
            if (aggregated_size > protocol::server::MAXIMUM_DATA_SIZE)
            {
                auto hash_size = std::min (meta_data->hashes().size() - offset, (i - 1) * 64);

                auto resp = client->read_chunks (
                        {{const_cast <char *> (meta_data->hashes().data() + offset), hash_size}});
                const auto& data = std::get<0>(resp.data);
                new_file.write (data.data(), data.size());
                offset = (i - 1) * 64;
                aggregated_size = meta_data->chunk_sizes()[i];
            }
        }

        if (aggregated_size > 0)
        {
            auto resp = client->read_chunks ({{const_cast <char *> (meta_data->hashes().data() + offset),
                                               meta_data->hashes().size() - offset}});

            const auto& data = std::get<0>(resp.data);
            m_size += data.size();
            new_file.write(data.data(), data.size());
        }

        std::filesystem::permissions(path,
                                    static_cast<std::filesystem::perms>(meta_data->permissions()),
                                    std::filesystem::perm_options::replace);
    }
    else if (meta_data->type() == uhv::uh_file_type::directory)
    {
        std::filesystem::create_directories(path);
        std::filesystem::permissions(path,
                                    static_cast<std::filesystem::perms>(meta_data->permissions()),
                                    std::filesystem::perm_options::replace);
    }

    else if (meta_data->type() == uhv::uh_file_type::none)
    {
        throw std::runtime_error("Unknown file type encountered: " + meta_data->path().string());
    }

    std::filesystem::permissions(path,
                                 static_cast<std::filesystem::perms>(meta_data->permissions()),
                                 std::filesystem::perm_options::replace);
}

// ---------------------------------------------------------------------

void download::spawn_threads()
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
                    add_result((*item)->path());
                }
                catch (const std::exception& e)
                {
                    add_result((*item)->path(), e.what());
                }
            }
        });
    }
}

// ---------------------------------------------------------------------

const std::map<std::filesystem::path, std::optional<std::string>>& download::results() const
{
    return m_results;
}

// ---------------------------------------------------------------------

std::size_t download::size() const
{
    return m_size;
}

// ---------------------------------------------------------------------

void download::add_result(const std::filesystem::path& p,
                          const std::optional<std::string>& error)
{
    const std::lock_guard<std::mutex> lock(m_result_mutex);
    m_results[p] = error;
}

// ---------------------------------------------------------------------

} // namespace uh::client
