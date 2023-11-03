#include "list_multi_part_uploads_request.h"

namespace uh::cluster::rest::http::model
{

    list_multi_part_uploads_request::list_multi_part_uploads_request(const http::request_parser<http::empty_body>& recv_req,
                                                                     std::unique_ptr<rest::http::URI> uri) : http_request(recv_req, std::move(uri))
    {
        *this = recv_req;
    }

    list_multi_part_uploads_request& list_multi_part_uploads_request::operator = (const http::request_parser<http::empty_body>& recv_req)
    {
        const auto& header_list = recv_req.get();

        std::string delimiter = m_uri->get_query_string_value("delimiter");
        if (!delimiter.empty())
        {
            m_delimiter  = std::move(delimiter);
            m_delimiterHasBeenSet = true;
        }

        std::string encoding_type = m_uri->get_query_string_value("encoding-type");
        if (!encoding_type.empty())
        {
            m_encodingType  = std::move(encoding_type);
            m_encodingTypeHasBeenSet = true;
        }

        std::string key_marker = m_uri->get_query_string_value("key-marker");
        if (!key_marker.empty())
        {
            m_keyMarker = std::move(key_marker);
            m_keyMarkerHasBeenSet = true;
        }

        std::string max_uploads = m_uri->get_query_string_value("max-uploads");
        if (!max_uploads.empty())
        {
            m_maxUploads  = std::stoi(max_uploads);
            m_maxUploadsHasBeenSet = true;
        }

        std::string prefix = m_uri->get_query_string_value("prefix");
        if (!prefix.empty())
        {
            m_prefix  = std::move(prefix);
            m_prefixHasBeenSet = true;
        }

        std::string upload_id = m_uri->get_query_string_value("upload-id-marker");
        if (!upload_id.empty())
        {
            m_uploadIdMarker  = std::move(upload_id);
            m_uploadIdMarkerHasBeenSet = true;
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

        return *this;
    }

    std::map<std::string, std::string> list_multi_part_uploads_request::get_request_specific_headers() const
    {
        std::map<std::string, std::string> headers;
        std::stringstream ss;

        if(m_expectedBucketOwnerHasBeenSet)
        {
            ss << m_expectedBucketOwner;
            headers.emplace("x-amz-expected-bucket-owner",  ss.str());
            ss.str("");
        }

        if(m_requestPayerHasBeenSet)
        {
            ss << m_requestPayer;
            headers.emplace("x-amz-request-payer",  ss.str());
            ss.str("");
        }

        return headers;
    }

} // uh::cluster::http::rest::model
