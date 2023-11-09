#include "custom_error_response_exception.h"
#include "iostream"

namespace uh::cluster::rest::http::model
{

    custom_error_response_exception::custom_error_response_exception(http::response<http::string_body> res, std::string&& body) : m_error(std::move(res))
    {
        m_error.set(http::field::server, "UltiHash");
        m_error.set(boost::beast::http::field::content_type, "application/xml");
        m_error.body() = std::move(body);
    }

    custom_error_response_exception::custom_error_response_exception(http::response<http::string_body> res) : m_error(std::move(res))
    {}

    [[nodiscard]] const http::response<http::string_body>& custom_error_response_exception::get_response_specific_object()
    {
        m_error.prepare_payload();

        std::cout << m_error.base() << std::endl;

        return m_error;
    }

} // uh::cluster::rest::http::model
