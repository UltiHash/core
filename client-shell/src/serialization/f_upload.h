#ifndef SERIALIZATION_F_UPLOAD_H
#define SERIALIZATION_F_UPLOAD_H

#include "../common/thread_manager.h"
#include "common/f_meta_data.h"
#include "../common/job_queue.h"

namespace uh::client::serialization
{

// ---------------------------------------------------------------------

class f_upload : public common::thread_manager
{
public:

    // ------------------------------------------------- CLASS FUNCTIONS
    explicit f_upload(common::job_queue& jq, size_t num_threads=1);
    ~f_upload() override;
    void spawn_threads() override;
    void fupload();

    // ------------------------------------------------- GETTERS


    // ------------------------------------------------- SETTERS


private:
    common::job_queue& m_input_jq;
    common::job_queue& m_output_jq;
};

// ---------------------------------------------------------------------

} // namespace uh::client::serialization

#endif
