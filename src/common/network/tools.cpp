#include "tools.h"

#include <boost/asio.hpp>

namespace asio = boost::asio;

namespace uh::cluster {

std::list<asio::ip::tcp::endpoint> resolve(const std::string& address,
                                           uint16_t port) {
    asio::io_context io_service;
    asio::ip::tcp::resolver resolver(io_service);

    auto results = resolver.resolve(address, std::to_string(port));
    return std::list<asio::ip::tcp::endpoint>(results.cbegin(), results.cend());
}

} // namespace uh::cluster
