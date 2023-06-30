#ifndef CLIENT_DOWNLOAD_H
#define CLIENT_DOWNLOAD_H

#include <protocol/client_pool.h>
#include <uhv/job_queue.h>
#include "thread_manager.h"
#include <fstream>
#include <map>


namespace uh::client
{

// ---------------------------------------------------------------------

class download : public thread_manager
{
public:

    download(protocol::client_pool& cl_pool,
             uhv::job_queue<std::unique_ptr<uhv::meta_data>>& jq,
             std::filesystem::path dest_path,
             unsigned int num_threads = 1);
    ~download() override;

    void spawn_threads() override;

    void join();
    [[nodiscard]] const std::map<std::filesystem::path, std::optional<std::string>>& results() const;

    std::size_t size() const;
private:
    void add_result(const std::filesystem::path& p,
                    const std::optional<std::string>& error = std::nullopt);

    void download_file(std::unique_ptr<uhv::meta_data>& meta_data);

    uhv::job_queue<std::unique_ptr<uhv::meta_data>>& m_input_jq;
    uh::protocol::client_pool& m_client_pool;
    std::filesystem::path m_dest_path;

    std::map<std::filesystem::path, std::optional<std::string>> m_results;
    std::mutex m_result_mutex;
    std::atomic<std::size_t> m_size;
};

// ---------------------------------------------------------------------

} // namespace uh::client

#endif
