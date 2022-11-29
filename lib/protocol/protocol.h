#ifndef PROTOCOL_PROTOCOL_H
#define PROTOCOL_PROTOCOL_H

#include <net/connection.h>
#include <memory>


namespace uh::protocol
{

// ---------------------------------------------------------------------

class protocol
{
public:
    virtual ~protocol() = default;
    virtual void handle(std::shared_ptr<net::connection> client) = 0;
};

// ---------------------------------------------------------------------

} // namespace uh::protocol

#endif
