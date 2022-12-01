#ifndef SERVER_DB_PROTOCOL_FACTORY_H
#define SERVER_DB_PROTOCOL_FACTORY_H

#include <util/factory.h>

#include "protocol.h"
#include "storage_backend.h"


namespace uh::dbn
{

// ---------------------------------------------------------------------

class protocol_factory : public util::factory<uh::protocol::protocol>
{
public:
    protocol_factory(storage_backend_interface& storage);

    virtual std::unique_ptr<uh::protocol::protocol> create() const override;

private:
    storage_backend_interface& m_storage;
};

// ---------------------------------------------------------------------

} // namespace uh::dbn

#endif
