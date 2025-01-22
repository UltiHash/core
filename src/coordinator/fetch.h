#pragma once

#include <boost/asio.hpp>

#include "common/types/common_types.h"

namespace uh::cluster {

coro<std::string> fetch_response_body(boost::asio::io_context& io_context);

coro<std::string> fetch_response_body(boost::asio::io_context& io_context,
                                      const std::string& url);

} // namespace uh::cluster
