#ifndef SERIALIZATION_F_UPLOAD_H
#define SERIALIZATION_F_UPLOAD_H

#include <fstream>
#include <latch>
#include <protocol/client_pool.h>
#include "../common/thread_manager.h"
#include "../common/job_queue.h"


namespace uh::client::serialization
{

// ---------------------------------------------------------------------

class f_upload : public common::thread_manager
{
public:

    f_upload(std::unique_ptr<protocol::client_pool>&,
            common::job_queue<std::unique_ptr<uhv::f_meta_data>>&,
            common::job_queue<std::unique_ptr<uhv::f_meta_data>>&,
            unsigned int=1);
    ~f_upload() override;

    void spawn_threads() override;
    void upload_files(std::unique_ptr<uhv::f_meta_data>&, protocol::client_pool::handle&);

private:
    common::job_queue<std::unique_ptr<uhv::f_meta_data>>& m_input_jq;
    common::job_queue<std::unique_ptr<uhv::f_meta_data>>& m_output_jq;
    std::unique_ptr<uh::protocol::client_pool>& m_client_pool;
};

// ---------------------------------------------------------------------

} // namespace uh::client::serialization

#endif
