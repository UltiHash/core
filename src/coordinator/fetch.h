#pragma once

#include <boost/asio.hpp>
#include <common/types/common_types.h>

namespace uh::cluster {

coro<std::string> fetch_response_body(boost::asio::io_context& io_context,
                                      const std::string& url,
                                      const std::string& username = "",
                                      const std::string& password = "");

coro<std::string> exponential_backoff(boost::asio::io_context& io_context,
                                      std::function<coro<std::string>()> task);

} // namespace uh::cluster
