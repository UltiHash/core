#include "list_multi_part_uploads_request.h"

namespace uh::cluster::rest::http::model {

list_multi_part_uploads_request::list_multi_part_uploads_request(
    const http::request_parser<http::empty_body>& recv_req,
    std::unique_ptr<rest::http::URI> uri)
    : http_request(recv_req, std::move(uri)) {
    *this = recv_req;
}

list_multi_part_uploads_request& list_multi_part_uploads_request::operator=(
    const http::request_parser<http::empty_body>& recv_req) {
    const auto& header_list = recv_req.get();

    auto URI = *m_uri;

    if (URI.query_string_exists("prefix")) {
        m_prefix = URI.get_query_string_value("prefix");
        if (!m_prefix.empty())
            m_prefixHasBeenSet = true;
    }

    if (URI.query_string_exists("delimiter")) {
        m_delimiter = URI.get_query_string_value("delimiter");
        if (!m_delimiter.empty())
            m_delimiterHasBeenSet = true;
    }

    if (URI.query_string_exists("encoding-type")) {
        m_encodingType = URI.get_query_string_value("encoding-type");
        if (!m_encodingType.empty())
            m_encodingTypeHasBeenSet = true;
    }

    if (URI.query_string_exists("key-marker")) {
        m_keyMarker = URI.get_query_string_value("key-marker");
        if (!m_keyMarker.empty())
            m_keyMarkerHasBeenSet = true;
    }

    if (URI.query_string_exists("max-uploads")) {
        m_maxUploads = std::stoi(URI.get_query_string_value("max-uploads"));
        if (m_maxUploads > -1)
            m_maxUploadsHasBeenSet = true;
    }

    const auto& request_payer_l = header_list.find("x-amz-request-payer");
    const auto& request_payer_u = header_list.find("X-Amz-Request-Payer");
    if (request_payer_l != header_list.end() ||
        request_payer_u != header_list.end()) {
        m_requestPayer = (request_payer_l != header_list.end())
                             ? request_payer_l->value()
                             : request_payer_u->value();
        m_requestPayerHasBeenSet = true;
    }

    const auto& expected_bucket_owner_l =
        header_list.find("x-amz-expected-bucket-owner");
    const auto& expected_bucket_owner_u =
        header_list.find("X-Amz-Expected-Bucket-Owner");
    if (expected_bucket_owner_l != header_list.end() ||
        expected_bucket_owner_u != header_list.end()) {
        m_expectedBucketOwner = (expected_bucket_owner_l != header_list.end())
                                    ? expected_bucket_owner_l->value()
                                    : expected_bucket_owner_u->value();
        m_expectedBucketOwnerHasBeenSet = true;
    }

    return *this;
}

std::map<std::string, std::string>
list_multi_part_uploads_request::get_request_specific_headers() const {
    std::map<std::string, std::string> headers;
    std::stringstream ss;

    if (m_expectedBucketOwnerHasBeenSet) {
        ss << m_expectedBucketOwner;
        headers.emplace("x-amz-expected-bucket-owner", ss.str());
        ss.str("");
    }

    if (m_requestPayerHasBeenSet) {
        ss << m_requestPayer;
        headers.emplace("x-amz-request-payer", ss.str());
        ss.str("");
    }

    return headers;
}

} // namespace uh::cluster::rest::http::model
