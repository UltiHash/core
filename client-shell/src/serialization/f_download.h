#ifndef SERIALIZATION_F_DOWNLOAD_H
#define SERIALIZATION_F_DOWNLOAD_H

#include <protocol/client_pool.h>
#include <uhv/job_queue.h>
#include "../common/thread_manager.h"
#include <fstream>
#include <map>


namespace uh::client::serialization
{

// ---------------------------------------------------------------------

class f_download : public common::thread_manager
{
public:

    f_download(protocol::client_pool& cl_pool,
               uhv::job_queue<std::unique_ptr<uhv::f_meta_data>>& jq,
               std::filesystem::path dest_path,
               unsigned int num_threads = 1);
    ~f_download() override;

    void spawn_threads() override;

    void join();
    const std::map<std::filesystem::path, std::optional<std::string>>& results() const;

private:
    void add_result(const std::filesystem::path& p,
                    const std::optional<std::string>& error = std::nullopt);

    void download_file(std::unique_ptr<uhv::f_meta_data>& f_meta_data);

    uhv::job_queue<std::unique_ptr<uhv::f_meta_data>>& m_input_jq;
    uh::protocol::client_pool& m_client_pool;
    std::filesystem::path m_dest_path;

    std::map<std::filesystem::path, std::optional<std::string>> m_results;
    std::mutex m_result_mutex;
};

// ---------------------------------------------------------------------

} // namespace uh::client::serialization

#endif
