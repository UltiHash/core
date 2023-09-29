#pragma once

#include <string>

namespace uh::cluster::rest::http
{

    enum class http_method
    {
        HTTP_GET,
        HTTP_POST,
        HTTP_DELETE,
        HTTP_PUT,
        HTTP_HEAD,
        HTTP_PATCH
    };

    std::string get_name_for_http_method(http_method method);

} // uh::cluster::rest::http