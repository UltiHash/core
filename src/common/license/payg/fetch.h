#pragma once

#include <common/license/payg/exp_backoff.h>

#include <boost/asio.hpp>
#include <common/types/common_types.h>

namespace uh::cluster {

coro<std::string> fetch_response_body(boost::asio::io_context& io_context,
                                      const std::string& url,
                                      const std::string& username = "",
                                      const std::string& password = "");

/**
 * Exception handler for this fetch module.
 *
 * @return `true` means to "retry with exponential backoff"
 */
backoff_action fetch_exception_handler(const std::exception& e);

} // namespace uh::cluster
