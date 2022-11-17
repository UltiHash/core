#ifndef SERVER_AGENCY_PROTOCOL_H
#define SERVER_AGENCY_PROTOCOL_H

#include "connection.h"
#include <boost/asio.hpp>

#include <memory>


using namespace boost::asio::ip;

namespace uh::an
{

// ---------------------------------------------------------------------

class protocol
{
public:
    virtual ~protocol() = default;
    virtual void handle(std::shared_ptr<connection> client) = 0;
};

// ---------------------------------------------------------------------

class uh_protocol : public protocol
{
public:
    virtual void handle(std::shared_ptr<connection> client) override;
};

// ---------------------------------------------------------------------

} // namespace uh::an

#endif
