#pragma once

#include <boost/beast/http.hpp>
#include <string>

namespace uh::cluster::rest::http {

enum class http_method {
    HTTP_GET,
    HTTP_POST,
};

http_method get_http_method_from_beast(boost::beast::http::verb method);

} // namespace uh::cluster::rest::http
