#ifndef SERIALIZATION_F_DOWNLOAD_H
#define SERIALIZATION_F_DOWNLOAD_H

#include <protocol/client_pool.h>
#include <uhv/job_queue.h>
#include "../common/thread_manager.h"
#include <fstream>


namespace uh::client::serialization
{

// ---------------------------------------------------------------------

class f_download : public common::thread_manager
{
public:

    f_download(std::unique_ptr<protocol::client_pool>& cl_pool,
               uhv::job_queue<std::unique_ptr<uhv::f_meta_data>>& jq,
               std::filesystem::path dest_path,
               unsigned int num_threads = 1);
    ~f_download() override;

    void spawn_threads() override;
    static void download_files(std::unique_ptr<uhv::f_meta_data>& f_meta_data,
                               protocol::client_pool::handle& client_handle);

private:
    uhv::job_queue<std::unique_ptr<uhv::f_meta_data>>& m_input_jq;
    std::unique_ptr<uh::protocol::client_pool>& m_client_pool;
    std::filesystem::path m_dest_path;
};

// ---------------------------------------------------------------------

} // namespace uh::client::serialization

#endif
