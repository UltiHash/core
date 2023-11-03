#include "list_objectsv2_request.h"

namespace uh::cluster::rest::http::model
{

    list_objectsv2_request::list_objectsv2_request(const http::request_parser<http::empty_body>& recv_req,
                                                   std::unique_ptr<rest::http::URI> uri) : http_request(recv_req, std::move(uri))
    {
        *this = recv_req;
    }

    list_objectsv2_request& list_objectsv2_request::operator = (const http::request_parser<http::empty_body>& recv_req)
    {
        const auto& header_list = recv_req.get();

        if (m_uri->query_string_exists("delimiter"))
        {
            m_delimiter  = m_uri->get_query_string_value("delimiter");
            m_delimiterHasBeenSet = true;
        }

        if (m_uri->query_string_exists("encoding-type"))
        {
            m_encodingType  = m_uri->get_query_string_value("encoding-type");
            m_encodingTypeHasBeenSet = true;
        }

        if (m_uri->query_string_exists("max-keys"))
        {
            m_maxKeys  = std::stoi(m_uri->get_query_string_value("max-keys"));
            m_maxKeysHasBeenSet = true;
        }

        if (m_uri->query_string_exists("prefix"))
        {
            m_prefix  = m_uri->get_query_string_value("prefix");
            m_prefixHasBeenSet = true;
        }

        if (m_uri->query_string_exists("continuation-token"))
        {
            m_continuationToken  = m_uri->get_query_string_value("continuation-token");
            m_continuationTokenHasBeenSet = true;
        }

        if (m_uri->query_string_exists("fetch-owner"))
        {
            m_fetchOwner  = m_uri->get_query_string_value("fetch-owner");
            m_fetchOwnerHasBeenSet = true;
        }

        if (m_uri->query_string_exists("start-after"))
        {
            m_startAfter  = m_uri->get_query_string_value("start-after");
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
