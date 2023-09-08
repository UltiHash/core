#include "s3_authenticator.h"
#include <map>
#include <openssl/sha.h>
#include <iostream>
#include <chrono>
#include <iomanip>
#include "s3_parser.h"

namespace uh::cluster {

//------------------------------------------------------------------------------

    extern s3_fields s3_field_to_enum(const std::string &field);

//------------------------------------------------------------------------------

    s3_authenticator::s3_authenticator(const http::request_parser<http::string_body>& received_request, parsed_request_wrapper& parsed_request) :
    m_parsed_request(parsed_request), m_received_request(received_request)
    {
        std::string_view authorization_string = m_parsed_request.http_parsed_fields[http_fields::authorization];

        auto credential_index = authorization_string.find("Credential=") + 11;
        auto slash_index = authorization_string.find_first_of('/', credential_index);

        if (credential_index != std::string::npos && slash_index != std::string::npos)
        {
            m_access_key = authorization_string.substr(credential_index, slash_index - credential_index);
        }

        auto signature_index = authorization_string.find("Signature=") + 10;
        if (signature_index != std::string::npos)
        {
            m_signature = authorization_string.substr(signature_index);
        }

        auto signedheaders_index = authorization_string.find("SignedHeaders=") + 14;
        if (signedheaders_index != std::string::npos)
        {
            auto semi_colon_index = authorization_string.find_first_of(';', signedheaders_index);
            while (semi_colon_index != std::string::npos)
            {
                m_signed_headers.emplace(authorization_string.substr(signedheaders_index, semi_colon_index - signedheaders_index));
                signedheaders_index = semi_colon_index+1;
                semi_colon_index = authorization_string.find_first_of(';', signedheaders_index);
            }
            m_signed_headers.emplace(authorization_string.substr(signedheaders_index, authorization_string.find_first_of(',', signedheaders_index) - signedheaders_index));
        }

        if (m_signature.empty() || m_signed_headers.empty() || m_access_key.empty())
            throw std::runtime_error("invalid authorization header given.");

        if (m_parsed_request.http_parsed_fields.contains(http_fields::content_type))
        {
            bool contains_string = std::ranges::any_of(m_signed_headers,[](const std::string& str)
            {
                return str == "content-type";
            });
            if (!contains_string)
                throw std::runtime_error("content-type must also be included in signed headers");
        }

        std::set<s3_fields> signed_headers_set;
        for (const auto& header : m_signed_headers)
        {
            signed_headers_set.emplace(s3_field_to_enum(header));
        }
        for (const auto& value : m_parsed_request.s3_parsed_fields)
        {
            if (! signed_headers_set.contains(value.first))
                throw std::runtime_error("signed headers must include all the x-amz tags given in the request");

        }

    }

//------------------------------------------------------------------------------

    std::string
    s3_authenticator::get_canonical_uri() const
    {
        return std::string('/' + m_parsed_request.bucket_id + '/' + m_parsed_request.object_key + '\n');
    }

//------------------------------------------------------------------------------

    std::string
    s3_authenticator::get_canonical_query_string()
    {
        return {'\n'};
    }

//------------------------------------------------------------------------------

    std::string
    s3_authenticator::get_headers()
    {
        std::multimap<std::string, std::string> lexically_sorted_headers;

        // iterate through all headers
        for (const auto& header : m_received_request.get() )
        {
            std::string header_name = header.name_string();
            for (char& c : header_name)
            {
                c = std::tolower(static_cast<unsigned char>(c));
            }

            lexically_sorted_headers.emplace(header_name, header.value());
        }

        std::string canonical_header_string {};
        std::string header_list_string {};

        // TODO: what happens on multiple same headers, do we convert it to one header with multiple values?
        for (const auto& pair: lexically_sorted_headers)
        {
            if (m_signed_headers.contains(pair.first))
            {
                canonical_header_string += pair.first + ":" + pair.second + "\n";
                header_list_string += pair.first + ";";
            }
        }
        header_list_string.pop_back();
        header_list_string += '\n';

        return {canonical_header_string + header_list_string};
    }

//------------------------------------------------------------------------------

    std::string
    sha_256(const std::string_view& payload)
    {
        if (payload.empty())
        {
            return {"e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"};
        }
        else
        {
            unsigned char result[SHA256_DIGEST_LENGTH];
            SHA256((unsigned char *) payload.data(), payload.size(), result);

            return {reinterpret_cast<char*>(result), SHA256_DIGEST_LENGTH};
        }
    }

//------------------------------------------------------------------------------

    std::string
    s3_authenticator::get_canonical_request()
    {
        std::string canonical_request {};
        switch (m_parsed_request.verb)
        {
            case http_fields::put:
                canonical_request.append("PUT\n");
                break;
            case http_fields::get:
                canonical_request.append("GET\n");
                break;
            case http_fields::delete_:
                canonical_request.append("DELETE\n");
                break;
            default:
                throw std::runtime_error("unknown verb encountered.");
        }


        canonical_request += get_canonical_uri() + get_canonical_query_string() + get_headers() + sha_256(m_parsed_request.body);

        return canonical_request;
    }

//------------------------------------------------------------------------------

    std::stringstream
    ISO_8601_timestamp()
    {
        auto currentTime = std::chrono::system_clock::now();
        std::time_t timeT = std::chrono::system_clock::to_time_t(currentTime);
        std::tm* timeInfo = std::gmtime(&timeT);

        std::stringstream iso8601Time;
        iso8601Time << std::put_time(timeInfo, "%Y-%m-%dT%H:%M:%SZ");

        return iso8601Time;
    }

//------------------------------------------------------------------------------

    std::string
    s3_authenticator::get_string_to_sign()
    {
        std::cout << get_canonical_request() << std::endl;
        return { "AWS4-HMAC-SHA256\n" + sha_256(get_canonical_request()) };
    }

//------------------------------------------------------------------------------

    void
    s3_authenticator::authenticate()
    {
        get_string_to_sign();
    }

//------------------------------------------------------------------------------

} // namespace uh::cluster