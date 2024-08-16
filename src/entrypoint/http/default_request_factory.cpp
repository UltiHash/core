#include "default_request_factory.h"

using namespace boost::asio;

namespace uh::cluster::ep::http {

coro<std::unique_ptr<http_request>>
default_request_factory::create(ip::tcp::socket& s) {
    return http_request::create(s);
}

} // namespace uh::cluster::ep::http
