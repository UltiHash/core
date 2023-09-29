#include "http_types.h"
#include <stdexcept>

namespace uh::cluster::rest::http
{

    std::string get_name_for_http_method(http_method method)
    {
        switch (method)
        {
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
                throw std::runtime_error("unknown method");
        }
    }

} // namespace uh::cluster::rest::http