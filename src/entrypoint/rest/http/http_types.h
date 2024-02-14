#pragma once

#include <boost/beast/http.hpp>
#include <string>

namespace uh::cluster::rest::http {

enum class http_method {
    HTTP_GET,
    HTTP_POST,
    HTTP_DELETE,
    HTTP_PUT,
    HTTP_HEAD,
    HTTP_PATCH
};

const char* get_name_for_http_method(http_method method);

http_method get_http_method_from_beast(boost::beast::http::verb method);

} // namespace uh::cluster::rest::http
