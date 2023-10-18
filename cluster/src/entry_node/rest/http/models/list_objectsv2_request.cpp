#include "list_objectsv2_request.h"

namespace uh::cluster::rest::http::model
{

    list_objectsv2_request::list_objectsv2_request(const http::request_parser<http::empty_body>& recv_req) : http_request(recv_req)
    {
        *this = recv_req;
    }

    list_objectsv2_request& list_objectsv2_request::operator = (const http::request_parser<http::empty_body>& recv_req)
    {
        const auto& header_list = recv_req.get();

        std::string delimiter = m_uri.get_query_string_value("delimiter");
        if (!delimiter.empty())
        {
            m_delimiter  = std::move(delimiter);
            m_delimiterHasBeenSet = true;
        }

        std::string encoding_type = m_uri.get_query_string_value("encoding-type");
        if (!encoding_type.empty())
        {
            m_encodingType  = std::move(encoding_type);
            m_encodingTypeHasBeenSet = true;
        }

        std::string max_keys = m_uri.get_query_string_value("max-keys");
        if (!max_keys.empty())
        {
            m_maxKeys  = std::stoi(max_keys);
            m_maxKeysHasBeenSet = true;
        }

        std::string prefix = m_uri.get_query_string_value("prefix");
        if (!prefix.empty())
        {
            m_prefix  = std::move(prefix);
            m_prefixHasBeenSet = true;
        }

        std::string continuation_token = m_uri.get_query_string_value("continuation-token");
        if (!continuation_token.empty())
        {
            m_continuationToken  = std::move(continuation_token);
            m_continuationTokenHasBeenSet = true;
        }

        std::string fetch_owner = m_uri.get_query_string_value("fetch-owner");
        if (!fetch_owner.empty())
        {
            m_fetchOwner  = std::move(fetch_owner);
            m_fetchOwnerHasBeenSet = true;
        }

        std::string start_after = m_uri.get_query_string_value("start-after");
        if (!start_after.empty())
        {
            m_startAfter  = std::move(start_after);
            m_startAfterHasBeenSet = true;
        }

        const auto& request_payer_l = header_list.find("x-amz-request-payer");
        const auto& request_payer_u = header_list.find("X-Amz-Request-Payer");
        if (request_payer_l != header_list.end() || request_payer_u != header_list.end())
        {
            m_requestPayer = (request_payer_l != header_list.end()) ? request_payer_l->value() : request_payer_u->value();
            m_requestPayerHasBeenSet = true;
        }

        const auto& expected_bucket_owner_l = header_list.find("x-amz-expected-bucket-owner");
        const auto& expected_bucket_owner_u = header_list.find("X-Amz-Expected-Bucket-Owner");
        if (expected_bucket_owner_l != header_list.end() || expected_bucket_owner_u != header_list.end())
        {
            m_expectedBucketOwner = (expected_bucket_owner_l != header_list.end()) ? expected_bucket_owner_l->value() : expected_bucket_owner_u->value();
            m_expectedBucketOwnerHasBeenSet = true;
        }

        const auto& optional_object_attributes_l = header_list.find("x-amz-optional-object-attributes");
        const auto& optional_object_attributes_u = header_list.find("X-Amz-Optional-Object-Attributes");
        if (optional_object_attributes_l != header_list.end() || optional_object_attributes_u != header_list.end())
        {
            m_optionalObjectAttributes.push_back((optional_object_attributes_l != header_list.end()) ? optional_object_attributes_l->value() : optional_object_attributes_u->value());
            m_optionalObjectAttributesHasBeenSet = true;
        }

        return *this;
    }

    std::map<std::string, std::string> list_objectsv2_request::get_request_specific_headers() const
    {
        std::map<std::string, std::string> headers;
        std::stringstream ss;

        if(m_requestPayerHasBeenSet)
        {
            ss << m_requestPayer;
            headers.emplace("x-amz-request-payer",  ss.str());
            ss.str("");
        }

        if(m_expectedBucketOwnerHasBeenSet)
        {
            ss << m_expectedBucketOwner;
            headers.emplace("x-amz-expected-bucket-owner",  ss.str());
            ss.str("");
        }

        if(m_optionalObjectAttributesHasBeenSet)
        {
            ss << m_optionalObjectAttributes.front();
            headers.emplace("x-amz-optional-object-attributes",  ss.str());
            ss.str("");
        }

        return headers;
    }

} // uh::cluster::http::rest::model
