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

    // TODO: remove
    context ctx;

    boost::asio::ip::tcp::endpoint peer;
    opentelemetry::context::Context context;
};

} // namespace uh::cluster
