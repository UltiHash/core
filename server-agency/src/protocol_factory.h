#ifndef SERVER_AGENCY_PROTOCOL_FACTORY_H
#define SERVER_AGENCY_PROTOCOL_FACTORY_H

#include <net/server.h>


namespace uh::an
{

// ---------------------------------------------------------------------

class protocol_factory : public util::factory<uh::protocol::protocol>
{
public:
    virtual std::unique_ptr<uh::protocol::protocol> create() const override;
};

// ---------------------------------------------------------------------

} // namespace uh::an

#endif
