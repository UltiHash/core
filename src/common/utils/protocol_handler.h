#pragma once

#include "common/network/messenger.h"

namespace uh::cluster {

class protocol_handler {
public:
    virtual coro<void> handle(boost::asio::ip::tcp::socket m) = 0;

    virtual ~protocol_handler() = default;
};

} // namespace uh::cluster
