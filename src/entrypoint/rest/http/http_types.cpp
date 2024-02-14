#include "http_types.h"
#include <stdexcept>

namespace uh::cluster::rest::http {

namespace beast = boost::beast::http;

const char* get_name_for_http_method(http_method method) {
    switch (method) {
    case http_method::HTTP_GET:
        return "GET";
    case http_method::HTTP_POST:
        return "POST";
    case http_method::HTTP_DELETE:
        return "DELETE";
    case http_method::HTTP_PUT:
        return "PUT";
    case http_method::HTTP_HEAD:
        return "HEAD";
    case http_method::HTTP_PATCH:
        return "PATCH";
    default:
        throw std::runtime_error("unknown http method");
    }
}

http_method get_http_method_from_beast(boost::beast::http::verb method) {
    switch (method) {
    case beast::verb::get:
        return http_method::HTTP_GET;
    case beast::verb::post:
        return http_method::HTTP_POST;
    case beast::verb::delete_:
        return http_method::HTTP_DELETE;
    case beast::verb::put:
        return http_method::HTTP_PUT;
    case beast::verb::head:
        return http_method::HTTP_HEAD;
    case beast::verb::patch:
        return http_method::HTTP_PATCH;
    default:
        throw std::runtime_error("unknown http method");
    }
}

} // namespace uh::cluster::rest::http