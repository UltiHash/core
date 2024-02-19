#include "http_types.h"
#include <stdexcept>

namespace uh::cluster::rest::http {

namespace beast = boost::beast::http;

http_method get_http_method_from_beast(boost::beast::http::verb method) {
    switch (method) {
    case beast::verb::get:
        return http_method::HTTP_GET;
    case beast::verb::post:
        return http_method::HTTP_POST;
    default:
        throw std::runtime_error("unknown http method");
    }
}

} // namespace uh::cluster::rest::http
