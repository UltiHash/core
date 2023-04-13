#ifndef SERIALIZATION_F_UPLOAD_H
#define SERIALIZATION_F_UPLOAD_H

#include <chunking/mod.h>
#include <fstream>
#include <uhv/job_queue.h>
#include <uhv/f_meta_data.h>
#include <protocol/client_pool.h>
#include "../common/thread_manager.h"


namespace uh::client::serialization
{

// ---------------------------------------------------------------------

class f_upload : public common::thread_manager
{
public:

    f_upload(protocol::client_pool& client_pool,
            uhv::job_queue<std::unique_ptr<uhv::f_meta_data>>& input_queue,
            uhv::job_queue<std::unique_ptr<uhv::f_meta_data>>& output_files,
            uh::client::chunking::file_chunker& chunker,
            unsigned int num_threads = 1);
    ~f_upload() override;

    void spawn_threads() override;

    void chunk_and_upload(std::unique_ptr<uhv::f_meta_data>& metadata,
                          protocol::client_pool::handle& client);

private:
    uhv::job_queue<std::unique_ptr<uhv::f_meta_data>>& m_input_jq;
    uhv::job_queue<std::unique_ptr<uhv::f_meta_data>>& m_output_jq;
    uh::protocol::client_pool& m_client_pool;
    uh::client::chunking::file_chunker& m_chunker;
};

// ---------------------------------------------------------------------

} // namespace uh::client::serialization

#endif
