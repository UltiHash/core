#pragma once

#include <boost/asio/ip/tcp.hpp>
#include <list>

namespace uh::cluster {

std::list<boost::asio::ip::tcp::endpoint> resolve(const std::string& address,
                                                  uint16_t port);

} // namespace uh::cluster
