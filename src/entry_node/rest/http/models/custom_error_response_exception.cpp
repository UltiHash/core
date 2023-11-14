#include "custom_error_response_exception.h"

#include <utility>
#include "iostream"

namespace uh::cluster::rest::http::model
{

    static const std::vector<std::pair<std::string, std::string>> error_messages =
            {
                    {"success", "success"},
                    {"fail", "fail"},
                    {"unknown", "unknown"},
                    {"NoSuchUpload", "upload id not found"},
                    {"MalformedXML", "xml is invalid"},
                    {"InvalidPart", "part not found"},
                    {"InvalidPartOrder", "part oder is not ascending"},
                    {"EntityTooSmall", "entity is too small"},
                    {"NoSuchBucket", "bucket not found"},
                    {"InvalidBucketName", "bucket name has invalid characters"}
            };

    static const std::pair<std::string, std::string> error_out_of_range = {"OutOfRange", "error out of range"};

    error::error(type t)
            : m_type(t)
    {}

    const std::pair<std::string, std::string>& error::message() const {

        auto ec = code();
        if (error_messages.size() <= ec) {
            return error_out_of_range;
        }

        return error_messages[ec];
    }

    uint32_t error::code() const {
        return static_cast<uint32_t>(m_type);
    }

    error::type error::operator*() const
    {
        return m_type;
    }

    const char* custom_error_response_exception::what() const noexcept
    {
        return m_error.message().second.c_str();
    }

    custom_error_response_exception::custom_error_response_exception(http::status status, error::type err) : m_res(status, 11), m_error(err)
    {
        m_res.set(http::field::server, "UltiHash");
        m_res.set(boost::beast::http::field::content_type, "application/xml");
    }

    [[nodiscard]] const http::response<http::string_body>& custom_error_response_exception::get_response_specific_object()
    {
        m_res.body() = "<Error>\n"
                       "<Code>" + m_error.message().first + "</Code>\n"
                       "<Message>" + m_error.message().second + "</Message>\n"
                       "</Error>";

        m_res.prepare_payload();

        std::cout << m_res.base() << std::endl;

        return m_res;
    }

} // uh::cluster::rest::http::model
