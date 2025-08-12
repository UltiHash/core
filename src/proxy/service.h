#pragma once

#include <common/network/server.h>
#include "config.h"

#include <boost/asio.hpp>


namespace uh::cluster::proxy {

class service {
public:
    service(boost::asio::io_context& ioc, const config& c);

private:
    boost::asio::io_context& m_ioc;
    server m_server;
};

} // namespace uh::cluster::proxy
