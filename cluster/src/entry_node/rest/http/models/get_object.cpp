#include "get_object.h"

namespace uh::cluster::rest::http::model
{

    get_object::get_object(const http::request_parser<http::empty_body> & recv_req) : http_request(recv_req)
    {
        // parse and set the received request parameters
        *this = recv_req;
    }

    get_object& get_object::operator = (const http::request_parser<http::empty_body>& recv_req)
    {
        const auto& header_list = recv_req.get();

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

        return *this;
    }

    std::map<std::string, std::string> get_object::get_request_specific_headers() const
    {
        std::map<std::string, std::string> headers;
        std::stringstream ss;

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

        return headers;
    }

} // uh::cluster::http::rest::model
