#pragma once

#include "common/types/common_types.h"
#include <boost/asio/ip/tcp.hpp>

namespace uh::cluster {

class protocol_handler {
public:
    virtual coro<void> handle(boost::asio::ip::tcp::socket m) = 0;

    virtual ~protocol_handler() = default;
};

} // namespace uh::cluster
