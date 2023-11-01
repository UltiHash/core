#include "delete_object_request.h"

namespace uh::cluster::rest::http::model
{

    delete_object_request::delete_object_request(const http::request_parser<http::empty_body>& recv_req,
                                                 std::unique_ptr<rest::http::URI> uri) : http_request(recv_req, std::move(uri))
    {
        // parse and set the received request parameters
        *this = recv_req;
    }

    delete_object_request& delete_object_request::operator = (const http::request_parser<http::empty_body>& recv_req)
    {
        const auto& header_list = recv_req.get();

        const auto& mfa_l = header_list.find("x-amz-mfa");
        const auto& mfa_u = header_list.find("X-Amz-Mfa");
        if (mfa_l != header_list.end() || mfa_u != header_list.end())
        {
            m_expectedBucketOwner = ( mfa_l != header_list.end() ) ? mfa_l->value() : mfa_u->value();
            m_expectedBucketOwnerHasBeenSet = true;
        }

        const auto& request_payer_l = header_list.find("x-amz-request-payer");
        const auto& request_payer_u = header_list.find("X-Amz-Request-Payer");
        if (request_payer_l != header_list.end() || request_payer_u != header_list.end())
        {
            m_expectedBucketOwner = ( request_payer_l != header_list.end() ) ? request_payer_l->value() : request_payer_u->value();
            m_expectedBucketOwnerHasBeenSet = true;
        }

        const auto& governance_l = header_list.find("x-amz-bypass-governance-retention");
        const auto& governance_u = header_list.find("X-Amz-Bypass-Governance-Retention");
        if (governance_l != header_list.end() || governance_u != header_list.end())
        {
            m_expectedBucketOwner = ( governance_l != header_list.end() ) ? governance_l->value() : governance_u->value();
            m_expectedBucketOwnerHasBeenSet = true;
        }

        const auto& expec_l = header_list.find("x-amz-expected-bucket-owner");
        const auto& expec_u = header_list.find("X-Amz-Expected-Bucket-Owner");
        if (expec_l != header_list.end() || expec_u != header_list.end())
        {
            m_expectedBucketOwner = ( expec_l != header_list.end() ) ? expec_l->value() : expec_u->value();
            m_expectedBucketOwnerHasBeenSet = true;
        }

        return *this;
    }

    std::map<std::string, std::string> delete_object_request::get_request_specific_headers() const
    {
        std::map<std::string, std::string> headers;
        std::stringstream ss;

        if (m_mFAHasBeenSet)
        {
            ss << m_mFA;
            headers.emplace("x-amz-mfa",  ss.str());
            ss.str("");
        }

        if (m_requestPayerHasBeenSet)
        {
            ss << m_requestPayer;
            headers.emplace("x-amz-request-payer",  ss.str());
            ss.str("");
        }

        if (m_bypassGovernanceRetentionHasBeenSet)
        {
            ss << m_bypassGovernanceRetention;
            headers.emplace("x-amz-bypass-governance-retention",  ss.str());
            ss.str("");
        }

        if (m_expectedBucketOwnerHasBeenSet)
        {
            ss << m_expectedBucketOwner;
            headers.emplace("x-amz-expected-bucket-owner",  ss.str());
            ss.str("");
        }

        return headers;
    }

} // uh::cluster::http::rest::model
