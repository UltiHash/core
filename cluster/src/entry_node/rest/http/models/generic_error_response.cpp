#include "generic_error_response.h"

namespace uh::cluster::rest::http::model
{

    error_response::error_response(http::response<http::string_body> res) : m_error(std::move(res))
    {}

    [[nodiscard]] const http::response<http::string_body>& error_response::get_response_specific_object()
    {
        m_error.set(http::field::server, "UltiHash");
        m_error.set(http::field::content_type, "text/html");
        m_error.prepare_payload();
        return m_error;
    }


} // uh::cluster::rest::http::model
