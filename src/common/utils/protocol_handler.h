#pragma once

#include "common/types/common_types.h"
#include <boost/asio/ip/tcp.hpp>

namespace uh::cluster {

struct protocol_handler {
    virtual coro<void> run() = 0;

    virtual ~protocol_handler() = default;
};

struct protocol_handler_factory {
    virtual std::unique_ptr<protocol_handler>
    create_handler(boost::asio::ip::tcp::socket&& s) = 0;

    virtual ~protocol_handler_factory() = default;
};

} // namespace uh::cluster
