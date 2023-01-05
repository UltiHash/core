#ifndef SERVER_DB_PROTOCOL_FACTORY_H
#define SERVER_DB_PROTOCOL_FACTORY_H

#include <util/factory.h>

#include "protocol.h"
#include "storage_backend.h"
#include "metrics.h"


namespace uh::dbn
{

// ---------------------------------------------------------------------

class protocol_factory : public util::factory<uh::protocol::protocol>
{
public:
    protocol_factory(storage_backend& storage, const dbn::metrics& metrics);

    virtual std::unique_ptr<uh::protocol::protocol> create() override;

private:
    storage_backend& m_storage;
    const dbn::metrics& m_metrics;
};

// ---------------------------------------------------------------------

} // namespace uh::dbn

#endif
