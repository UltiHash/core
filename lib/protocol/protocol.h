#ifndef PROTOCOL_PROTOCOL_H
#define PROTOCOL_PROTOCOL_H

#include <net/socket.h>
#include <memory>


namespace uh::protocol
{

// ---------------------------------------------------------------------

class protocol
{
protected:
    std::shared_ptr<net::socket> client_;
public:
    explicit protocol (std::shared_ptr<net::socket> client): client_(std::move (client)) {}

    virtual ~protocol() = default;
    virtual void handle() = 0;
};


class protocol_factory
{
public:
    virtual ~protocol_factory() = default;
    virtual std::unique_ptr<uh::protocol::protocol> create(std::shared_ptr<net::socket> client)  = 0;
};

// ---------------------------------------------------------------------

} // namespace uh::protocol

#endif
