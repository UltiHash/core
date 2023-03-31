#ifndef SERIALIZATION_RECOMPILATION_H
#define SERIALIZATION_RECOMPILATION_H

#include "../client_options/client_config.h"
#include "protocol/client_factory.h"
#include "protocol/client_pool.h"
#include "common/f_meta_data.h"
#include "common/job_queue.h"
#include "f_upload.h"
#include "f_download.h"
#include "f_traverse.h"
#include "f_serialization.h"
#include <chunking/mod.h>

namespace co = uh::client::option;

namespace uh::client::serialization
{

// ---------------------------------------------------------------------

class Recompilation
{
    public:
        Recompilation(const co::client_config&,
        const uh::client::chunking::chunking_config& ,
        std::unique_ptr<uh::protocol::client_pool>&&);

    private:
        void integrate();
        void retrieve();

    private:
        const co::client_config& m_client_config;
        const client::chunking::chunking_config& m_chunker_config;
        std::unique_ptr<uh::protocol::client_pool> m_client_pool;

};

// ---------------------------------------------------------------------

} // namespace uh::client::serialization

#endif
