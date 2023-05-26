#ifndef CLIENT_F_UPLOAD_H
#define CLIENT_F_UPLOAD_H

#include <uhv/job_queue.h>
#include <uhv/meta_data.h>
#include <protocol/client_pool.h>
#include <protocol/server.h>
#include <chunking/mod.h>
#include <algorithm>
#include "thread_manager.h"

#include <fstream>
#include <map>


namespace uh::client
{

// ---------------------------------------------------------------------

class f_upload : public thread_manager
{
public:


    f_upload(protocol::client_pool& client_pool,
            uhv::job_queue<std::unique_ptr<uhv::f_meta_data>>& input_queue,
            uhv::job_queue<std::unique_ptr<uhv::f_meta_data>>& output_files,
            uh::chunking::mod& chunking,
            std::filesystem::path uhv_path,
            unsigned int num_threads = 1);
    ~f_upload() override;

    void spawn_threads() override;
    void join();

    [[nodiscard]] const std::map<std::filesystem::path, std::optional<std::string>>& results() const;
    void send_statistics();
    void chunk_and_upload(std::unique_ptr<uhv::f_meta_data>& metadata,
                          protocol::client_pool::handle& client,
                          io::device& device);

private:
    void add_result(const std::filesystem::path& p,
                const std::optional<std::string>& error = std::nullopt);

    uhv::job_queue<std::unique_ptr<uhv::f_meta_data>>& m_input_jq;
    uhv::job_queue<std::unique_ptr<uhv::f_meta_data>>& m_output_jq;
    uh::protocol::client_pool& m_client_pool;
    uh::chunking::mod& m_chunking;
    std::filesystem::path m_uhv_path;

    std::atomic<std::uint64_t> m_uploaded_size;

    std::map<std::filesystem::path, std::optional<std::string>> m_results;
    std::mutex m_result_mutex;
};

// ---------------------------------------------------------------------

} // namespace uh::client

#endif
