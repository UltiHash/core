#include "get_object_request.h"

namespace uh::cluster::rest::http::model
{

    get_object_request::get_object_request(const http::request_parser<http::empty_body> & recv_req) : http_request(recv_req)
    {
        // parse and set the received request parameters
        *this = recv_req;
    }

    get_object_request& get_object_request::operator = (const http::request_parser<http::empty_body>& recv_req)
    {
        const auto& header_list = recv_req.get();

        const auto& if_match_l = header_list.find("if-match");
        const auto& if_match_u = header_list.find("If-Match");
        if (if_match_l != header_list.end() || if_match_u != header_list.end())
        {
            m_ifMatch = ( if_match_l != header_list.end() ) ? if_match_l->value() : if_match_u->value();
            m_ifMatchHasBeenSet = true;
        }

        const auto& if_modified_since_l = header_list.find("if-modified-since");
        const auto& if_modified_since_u = header_list.find("If-Modified-Since");
        if (if_modified_since_l != header_list.end() || if_modified_since_u != header_list.end())
        {
            m_ifModifiedSince = ( if_modified_since_l != header_list.end() ) ? if_modified_since_l->value() : if_modified_since_u->value();
            m_ifModifiedSinceHasBeenSet = true;
        }

        const auto& if_none_match_l = header_list.find("if-none-match");
        const auto& if_none_match_u = header_list.find("If-None-Match");
        if (if_none_match_l != header_list.end() || if_none_match_u != header_list.end())
        {
            m_ifNoneMatch = ( if_none_match_l != header_list.end() ) ? if_none_match_l->value() : if_none_match_u->value();
            m_ifNoneMatchHasBeenSet = true;
        }

        const auto& if_unmodified_since_l = header_list.find("if-modified-since");
        const auto& if_unmodified_since_u = header_list.find("If-Modified-Since");
        if (if_unmodified_since_l != header_list.end() || if_unmodified_since_u != header_list.end())
        {
            m_ifUnmodifiedSince = ( if_unmodified_since_l != header_list.end() ) ? if_unmodified_since_l->value() : if_unmodified_since_u->value();
            m_ifUnmodifiedSinceHasBeenSet = true;
        }

        const auto& range = header_list.find(boost::beast::http::field::range);
        if (range != header_list.end())
        {
            m_range = range->value();
            m_rangeHasBeenSet = true;
        }

        const auto& sse_customer_algorithm_l = header_list.find("x-amz-server-side-encryption-customer-algorithm");
        const auto& sse_customer_algorithm_u = header_list.find("X-Amz-Server-Side-Encryption-Customer-Algorithm");
        if (sse_customer_algorithm_l != header_list.end() || sse_customer_algorithm_u != header_list.end())
        {
            m_sSECustomerAlgorithm = (sse_customer_algorithm_l != header_list.end()) ? sse_customer_algorithm_l->value() : sse_customer_algorithm_u->value();
            m_sSECustomerAlgorithmHasBeenSet = true;
        }

        const auto& sse_customer_key_l = header_list.find("x-amz-server-side-encryption-customer-key");
        const auto& sse_customer_key_u = header_list.find("X-Amz-Server-Side-Encryption-Customer-Key");
        if (sse_customer_key_l != header_list.end() || sse_customer_key_u != header_list.end())
        {
            m_sSECustomerKey = (sse_customer_key_l != header_list.end()) ? sse_customer_key_l->value() : sse_customer_key_u->value();
            m_sSECustomerKeyHasBeenSet = true;
        }

        const auto& sse_customer_key_md5_l = header_list.find("x-amz-server-side-encryption-customer-key-MD5");
        const auto& sse_customer_key_md5_u = header_list.find("X-Amz-Server-Side-Encryption-Customer-Key-MD5");
        if (sse_customer_key_md5_l != header_list.end() || sse_customer_key_md5_u != header_list.end())
        {
            m_sSECustomerKeyMD5 = (sse_customer_key_md5_l != header_list.end()) ? sse_customer_key_md5_l->value() : sse_customer_key_md5_u->value();
            m_sSECustomerKeyMD5HasBeenSet = true;
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

        const auto& checksome_mode_l = header_list.find("x-amz-checksum-mode");
        const auto& checksome_mode_u = header_list.find("X-Amz-Checksum-Mode");
        if (checksome_mode_l != header_list.end() || checksome_mode_u != header_list.end())
        {
            m_checksumMode = (checksome_mode_l != header_list.end()) ? checksome_mode_l->value() : checksome_mode_u->value();
            m_checksumModeHasBeenSet = true;
        }

        return *this;
    }

    std::map<std::string, std::string> get_object_request::get_request_specific_headers() const
    {
        std::map<std::string, std::string> headers;
        std::stringstream ss;

        if(m_ifMatchHasBeenSet)
        {
            ss << m_ifMatch;
            headers.emplace("if-match",  ss.str());
            ss.str("");
        }

        if(m_ifModifiedSinceHasBeenSet)
        {
            ss << m_ifModifiedSince;
            headers.emplace("if-modified-since",  ss.str());
            ss.str("");
        }

        if(m_ifNoneMatchHasBeenSet)
        {
            ss << m_ifNoneMatch;
            headers.emplace("if-none-match",  ss.str());
            ss.str("");
        }

        if(m_ifUnmodifiedSinceHasBeenSet)
        {
            ss << m_ifUnmodifiedSince;
            headers.emplace("if-unmodified-since",  ss.str());
            ss.str("");
        }

        if(m_rangeHasBeenSet)
        {
            ss << m_range;
            headers.emplace("range",  ss.str());
            ss.str("");
        }

        if(m_sSECustomerAlgorithmHasBeenSet)
        {
            ss << m_sSECustomerAlgorithm;
            headers.emplace("x-amz-server-side-encryption-customer-algorithm",  ss.str());
            ss.str("");
        }

        if(m_sSECustomerKeyHasBeenSet)
        {
            ss << m_sSECustomerKey;
            headers.emplace("x-amz-server-side-encryption-customer-key",  ss.str());
            ss.str("");
        }

        if(m_sSECustomerKeyMD5HasBeenSet)
        {
            ss << m_sSECustomerKeyMD5;
            headers.emplace("x-amz-server-side-encryption-customer-key-md5",  ss.str());
            ss.str("");
        }

        if(m_requestPayerHasBeenSet)
        {
            ss << m_requestPayer;
            headers.emplace("x-amz-request-payer", ss.str());
            ss.str("");
        }

        if(m_expectedBucketOwnerHasBeenSet)
        {
            ss << m_expectedBucketOwner;
            headers.emplace("x-amz-expected-bucket-owner",  ss.str());
            ss.str("");
        }

        if(m_checksumModeHasBeenSet)
        {
            ss << m_checksumModeHasBeenSet;
            headers.emplace("x-amz-checksum-mode",  ss.str());
            ss.str("");
        }

        return headers;
    }

} // uh::cluster::http::rest::model
