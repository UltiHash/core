#ifndef PROTOCOL_PROTOCOL_H
#define PROTOCOL_PROTOCOL_H

#include "common.h"
#include <net/socket.h>
#include <memory>


namespace uh::protocol
{

// ---------------------------------------------------------------------

class protocol
{
public:
    virtual ~protocol() = default;
    virtual void handle() = 0;
};

// ---------------------------------------------------------------------

class protocol_factory
{
public:
    virtual ~protocol_factory() = default;
    virtual std::unique_ptr<uh::protocol::protocol> create(std::shared_ptr<net::socket> client) = 0;
};

// ---------------------------------------------------------------------

} // namespace uh::protocol

#endif
