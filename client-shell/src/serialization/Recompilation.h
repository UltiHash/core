#ifndef SERIALIZATION_RECOMPILATION_H
#define SERIALIZATION_RECOMPILATION_H

#include "../client_options/client_config.h"
#include "protocol/client_factory.h"
#include "protocol/client_pool.h"
#include "common/f_meta_data.h"
#include "common/job_queue.h"

namespace co = uh::client::option;

namespace uh::client::serialization
{

// ---------------------------------------------------------------------

class Recompilation
{
    public:
        Recompilation(const co::client_config& config, std::unique_ptr<uh::protocol::client_pool>&& factory);

    private:
        void integrate();
        void list();
        void retrieve();

    private:
        const co::client_config& m_config;
        std::unique_ptr<uh::protocol::client_pool> m_client_pool;

};

// ---------------------------------------------------------------------

} // namespace uh::client::serialization

#endif
