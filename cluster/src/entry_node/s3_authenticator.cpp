#include "s3_authenticator.h"
#include <map>

namespace uh::rest {

//------------------------------------------------------------------------------

    s3_authenticator::s3_authenticator(const http::request<http::string_body>& received_request, const uh::rest::parsed_request_wrapper &parsed_request) :
    m_parsed_request(parsed_request), m_received_request(received_request)
    {
    }

//------------------------------------------------------------------------------

    std::string
    s3_authenticator::get_canonical_uri()
    {
        return std::string('/' + m_parsed_request.bucket_id + '/' + m_parsed_request.object_key + '\n');
    }

//------------------------------------------------------------------------------

    std::string
    s3_authenticator::get_canonical_query_string()
    {
        return std::string("\n");
    }

//------------------------------------------------------------------------------

    std::string
    s3_authenticator::get_canonical_headers()
    {
        std::multimap<std::string, std::string> lexically_sorted_headers;

        // iterate through all headers
        for (const auto& header : m_received_request)
        {
            lexically_sorted_headers.emplace(header.name_string(), header.value());
        }
    }

//------------------------------------------------------------------------------

    std::string
    s3_authenticator::get_canonical_request()
    {
        std::string canonical_headers;
        switch (m_parsed_request.verb)
        {
            case put:
                canonical_headers.append("PUT");
                break;
            case get:
                canonical_headers.append("GET");
                break;
            case delete_:
                canonical_headers.append("DELETE");
                break;
            default:
                throw std::runtime_error("unknown verb encountered.");
        }

        canonical_headers.append(get_canonical_uri());
        canonical_headers.append(get_canonical_query_string());
        return canonical_headers;
    }

//------------------------------------------------------------------------------

    std::string
    s3_authenticator::get_string_to_sign()
    {
        return {};
    }

//------------------------------------------------------------------------------

    void
    s3_authenticator::authenticate()
    {

    }

//------------------------------------------------------------------------------

} // namespace uh::rest