#ifndef SERIALIZATION_RECOMPILATION_H
#define SERIALIZATION_RECOMPILATION_H

#include "../client_options/client_config.h"
#include "file_meta_data.h"
#include "job_queue.h"


namespace uh::client::serialization
{

// ---------------------------------------------------------------------

class Recompilation
{
    private:
        client_config m_config;
        job_queue<std::unique_ptr<file_meta_data>> m_fmeta_data_jq;

};

// ---------------------------------------------------------------------

} // namespace uh::client::serialization

#endif
