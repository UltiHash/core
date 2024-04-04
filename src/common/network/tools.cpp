#include "tools.h"

#include <boost/asio.hpp>

using namespace boost;

namespace uh::cluster {

std::list<asio::ip::tcp::endpoint> resolve(const std::string& address,
                                           uint16_t port) {
    asio::io_service io_service;
    asio::ip::tcp::resolver resolver(io_service);
    asio::ip::tcp::resolver::query query(address, std::to_string(port));

    auto results = resolver.resolve(query);
    return std::list<asio::ip::tcp::endpoint>(
        results.begin(), asio::ip::tcp::resolver::iterator());
}

} // namespace uh::cluster
