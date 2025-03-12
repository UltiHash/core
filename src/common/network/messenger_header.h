#pragma once

#include <common/telemetry/context.h>
#include <common/utils/common.h>

#include <boost/asio.hpp>
#include <opentelemetry/context/context.h>

namespace uh::cluster {

using size_type = size_t;

struct messenger_header {
    message_type type;
    size_type size;

    boost::asio::ip::tcp::endpoint peer;
};

} // namespace uh::cluster
