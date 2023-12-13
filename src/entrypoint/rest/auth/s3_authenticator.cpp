//#include "s3_authenticator.h"
//#include <map>
//#include <openssl/sha.h>
//#include <openssl/hmac.h>
//#include <chrono>
//#include <iomanip>
//#include "s3_parser.h"
//#include <iostream>
//
//#define EMPTY_SHA_256 "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
//#define NEWLINE '\n'
//
//namespace uh::cluster {
//
////------------------------------------------------------------------------------
//
//    extern s3_fields s3_field_to_enum(const std::string &field);
//    extern http_fields http_field_to_enum(const std::string &field);
//
////------------------------------------------------------------------------------
//
//    s3_authenticator::s3_authenticator(parsed_request_wrapper& parsed_request) :
//    m_parsed_request(parsed_request)
//    {
//
//        if (m_parsed_request.http_parsed_fields.find(http_fields::authorization) == m_parsed_request.http_parsed_fields.end())
//            throw std::runtime_error("no authorization header");
//
//        std::string_view authorization_string = m_parsed_request.http_parsed_fields[http_fields::authorization];
//
//        auto credential_index = authorization_string.find("Credential=");
//        auto signed_headers_index = authorization_string.find("SignedHeaders=");
//        auto signature_index = authorization_string.find("Signature=");
//
//        if (credential_index == std::string::npos || signed_headers_index == std::string::npos || signature_index == std::string::npos)
//            throw std::runtime_error("invalid authorization header");
//
//        m_algorithm = authorization_string.substr(0, credential_index - 1);
//
//        auto index_pointer = credential_index + 11;
//        for (int slashes = 0; slashes < 4; ++ slashes )
//        {
//            auto slash_index = authorization_string.find_first_of('/', index_pointer);
//            if (slash_index == std::string::npos)
//                throw std::runtime_error("invalid authorization header");
//
//
//            switch (slashes)
//            {
//                case 0:
//                    m_access_key = authorization_string.substr(index_pointer, slash_index - index_pointer);
//                    break;
//                case 1:
//                    m_date = authorization_string.substr(index_pointer, slash_index - index_pointer);
//                    break;
//                case 2:
//                    m_region = authorization_string.substr(index_pointer, slash_index - index_pointer);
//                    break;
//                case 3:
//                    m_service = authorization_string.substr(index_pointer, slash_index - index_pointer);
//                    break;
//            }
//
//            index_pointer = slash_index + 1;
//        }
//
//        index_pointer = signed_headers_index + 14;
//        auto semi_colon_index = authorization_string.find_first_of(";,", index_pointer);
//        while (semi_colon_index != std::string::npos)
//        {
//
//            m_signed_headers.emplace(authorization_string.substr(index_pointer, semi_colon_index - index_pointer));
//
//            if (authorization_string[semi_colon_index] == ',') [[unlikely]]
//                break;
//
//            index_pointer = semi_colon_index + 1;
//            semi_colon_index = authorization_string.find_first_of(";,", index_pointer);
//
//        }
//
//        m_signature = authorization_string.substr(signature_index + 10);
//
//        if ( m_algorithm.empty() || m_access_key.empty() || m_date.empty() || m_region.empty()
//             || m_service.empty() || m_signed_headers.empty() || m_signature.empty())
//            throw std::runtime_error("invalid authorization header");
//
//
//        if (m_parsed_request.http_parsed_fields.contains(http_fields::content_type))
//        {
//            // TODO: signed header is sorted alphabetically so if we don't find content-type within 'c' character we can stop searching
//            bool contains_string = std::ranges::any_of(m_signed_headers,[](const std::string& str)
//            {
//                return str == "content-type";
//            });
//            if (!contains_string)
//                throw std::runtime_error("content-type must also be included in signed headers");
//        }
//
//        std::set<s3_fields> signed_headers_set;
//        for (const auto& header : m_signed_headers)
//        {
//            signed_headers_set.emplace(s3_field_to_enum(header));
//        }
//        for (const auto& value : m_parsed_request.s3_parsed_fields)
//        {
//            if (! signed_headers_set.contains(value.first))
//                throw std::runtime_error("signed headers must include all the x-amz tags given in the request");
//
//        }
//
//    }
//
////------------------------------------------------------------------------------
//
////    std::string s3_authenticator::uri_encode() const
////    {
////
////    }
////
//////------------------------------------------------------------------------------
////
////    std::string s3_authenticator::trim() const
////    {
////
////    }
//
////------------------------------------------------------------------------------
//
//    std::string
//    s3_authenticator::get_canonical_uri() const
//    {
//        return std::string('/' + m_parsed_request.bucket_id + '/' + m_parsed_request.object_key + "\n");
//    }
//
////------------------------------------------------------------------------------
//
//    std::string
//    s3_authenticator::get_canonical_query_string() const
//    {
//        std::string canonical_query_string {};
//
//        std::size_t query_index;
//        std::size_t initial_index = 0;
//        do
//        {
//            query_index = m_parsed_request.query_string.find('&');
//            auto single_query_parameter = m_parsed_request.query_string.substr(initial_index, query_index - 1);
//
//            if (!single_query_parameter.empty() && single_query_parameter.find('=') == std::string::npos)
//                single_query_parameter += '=';
//
//            canonical_query_string += single_query_parameter + '&';
//
//            // TODO: uriencode and trim here
//
//
//            initial_index = query_index + 1;
//
//        } while (query_index != std::string::npos);
//        canonical_query_string.pop_back();
//
//        // TODO: sort parameters alphabetically by the key name
//
//        return canonical_query_string + "\n";
//    }
//
////------------------------------------------------------------------------------
//
//    std::string
//    to_hex(const std::string& sha)
//    {
//        if (sha.empty())
//        {
//            return {"e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"};
//        }
//        else
//        {
//            std::stringstream ss;
//            for (unsigned char i: sha)
//            {
//                ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(i);
//            }
//
//            return std::move(ss.str());
//        }
//    }
//
////------------------------------------------------------------------------------
//
//    std::string
//    sha_256(const std::string_view& payload)
//    {
//        if (payload.empty())
//        {
//            return {};
//        }
//        else
//        {
//            unsigned char result[SHA256_DIGEST_LENGTH];
//            SHA256((unsigned char *) payload.data(), payload.length(), result);
//            return {reinterpret_cast<char*>(result), SHA256_DIGEST_LENGTH};
//        }
//    }
//
////------------------------------------------------------------------------------
//
//    // not hex encoded
//    std::string
//    s3_authenticator::hmac_sha_256(const std::string& signing_key, const std::string& string_to_sign) const
//    {
//        unsigned char hmac_result[SHA256_DIGEST_LENGTH];
//        // TODO: HMAC can fail and return nullptr
//        HMAC(
//                EVP_sha256(),
//                signing_key.data(),
//                signing_key.length(),
//                reinterpret_cast<const unsigned char*>(string_to_sign.data()),
//                string_to_sign.length(),
//                hmac_result,
//                nullptr
//        );
//
//        return {reinterpret_cast<char*>(hmac_result), SHA256_DIGEST_LENGTH};
//    }
//
////------------------------------------------------------------------------------
//
//    std::string
//    s3_authenticator::get_headers() const
//    {
//        std::string canonical_header_string {};
//        std::string header_list_string {};
//
//        // TODO: what happens on multiple same headers, do we convert it to one header with multiple values?
//        for (const auto& header : m_signed_headers)
//        {
//            if (header.starts_with("x-"))
//            {
//                auto converted_s3_enum = s3_field_to_enum(header);
//                if (m_parsed_request.s3_parsed_fields.contains(converted_s3_enum))
//                {
//                    canonical_header_string += header + ":" + std::string(m_parsed_request.s3_parsed_fields[converted_s3_enum]) + "\n";
//                }
//                else
//                    throw std::runtime_error("one of the given signed header doesn't exists");
//            }
//            else
//            {
//                auto converted_http_enum = http_field_to_enum(header);
//                if (m_parsed_request.http_parsed_fields.contains(converted_http_enum))
//                {
//                    canonical_header_string += header + ":" + std::string(m_parsed_request.http_parsed_fields[converted_http_enum]) + "\n";
//                }
//                else
//                    throw std::runtime_error("one of the given signed header doesn't exists");
//            }
//            header_list_string += header + ";";
//        }
//
//        header_list_string.pop_back();
//        header_list_string += "\n";
//
//        return {canonical_header_string + NEWLINE + header_list_string};
//    }
//
////------------------------------------------------------------------------------
//
//    std::string
//    s3_authenticator::get_canonical_request() const
//    {
//        std::string canonical_request {};
//        switch (m_parsed_request.verb)
//        {
//            case http_fields::post:
//                canonical_request.append("POST\n");
//                break;
//            case http_fields::put:
//                canonical_request.append("PUT\n");
//                break;
//            case http_fields::get:
//                canonical_request.append("GET\n");
//                break;
//            case http_fields::delete_:
//                canonical_request.append("DELETE\n");
//                break;
//            default:
//                throw std::runtime_error("unknown verb encountered.");
//        }
//
//        if (m_parsed_request.body.empty())
//            canonical_request += get_canonical_uri() + get_canonical_query_string() + get_headers() + EMPTY_SHA_256;
//        else
//            canonical_request += get_canonical_uri() + get_canonical_query_string() + get_headers() + to_hex(sha_256(m_parsed_request.body));
//
//        return canonical_request;
//    }
//
////------------------------------------------------------------------------------
//
//    std::string
//    s3_authenticator::ISO_8601_timestamp() const
//    {
//        auto currentTime = std::chrono::system_clock::now();
//        std::time_t timeT = std::chrono::system_clock::to_time_t(currentTime);
//        std::tm* timeInfo = std::gmtime(&timeT);
//
//        std::stringstream iso8601Time;
//        iso8601Time << std::put_time(timeInfo, "%Y%m%dT%H%M%SZ");
//
//        return iso8601Time.str();
//    }
//
////------------------------------------------------------------------------------
//
//    std::string
//    s3_authenticator::get_scope() const
//    {
//        return get_formatted_date() + '/' + m_region + '/' + m_service + "/aws4_request";
//    }
//
////------------------------------------------------------------------------------
//
//    std::string
//    s3_authenticator::get_formatted_date() const
//    {
//        std::time_t t = std::time(nullptr);
//        std::tm tm = *std::localtime(&t);
//
//        char buffer[9];
//        std::strftime(buffer, sizeof(buffer), "%Y%m%d", &tm);
//
//        return {buffer, sizeof(buffer) - 1};
//    }
//
////------------------------------------------------------------------------------
//
//    std::string
//    s3_authenticator::get_string_to_sign() const
//    {
//        return "AWS4-HMAC-SHA256\n" + std::string(m_parsed_request.s3_parsed_fields[x_amz_date]) + "\n" + get_scope() + "\n" + to_hex(sha_256(get_canonical_request()));
//    }
//
////------------------------------------------------------------------------------
//
//    std::string
//    s3_authenticator::signing_key() const
//    {
//        auto first_hmac = hmac_sha_256("AWS4" + m_secret_key, get_formatted_date());
//        auto second_hmac = hmac_sha_256( first_hmac, m_region );
//        auto third_hmac = hmac_sha_256( second_hmac, m_service );
//        auto tmp = hmac_sha_256( third_hmac, "aws4_request" );
//        return tmp;
//    }
//
////------------------------------------------------------------------------------
//
//    void
//    s3_authenticator::authenticate() const
//    {
//        auto calculated_signature = to_hex(hmac_sha_256( signing_key(), get_string_to_sign() ));
//
//        if (m_signature != calculated_signature)
//            throw std::runtime_error("authentication failed");
//
//    }
//
////------------------------------------------------------------------------------
//
//} // namespace uh::cluster
