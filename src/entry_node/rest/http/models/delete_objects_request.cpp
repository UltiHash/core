#include "delete_objects_request.h"

namespace uh::cluster::rest::http::model
{

    delete_objects_request::delete_objects_request(const http::request_parser<http::empty_body>& recv_req,
                                                   std::unique_ptr<rest::http::URI> uri) : http_request(recv_req, std::move(uri))
    {
        *this = recv_req;
    }

    delete_objects_request& delete_objects_request::operator = (const http::request_parser<http::empty_body>& recv_req)
    {
        const auto& header_list = recv_req.get();

        const auto& mfa_l = header_list.find("x-amz-mfa");
        const auto& mfa_u = header_list.find("X-Amz-Mfa");
        if (mfa_l != header_list.end() || mfa_u != header_list.end())
        {
            m_mFA = ( mfa_l != header_list.end() ) ? mfa_l->value() : mfa_u->value();
            m_mFAHasBeenSet = true;
        }

        const auto& payer_l = header_list.find("x-amz-request-payer");
        const auto& payer_u = header_list.find("X-Amz-Request-Payer");
        if (payer_l != header_list.end() || payer_u != header_list.end())
        {
            m_requestPayer = ( payer_l != header_list.end() ) ? payer_l->value() : payer_u->value();
            m_requestPayerHasBeenSet = true;
        }

        const auto& bypassgovernance_l = header_list.find("x-amz-bypass-governance-retention");
        const auto& bypassgovernance_u = header_list.find("X-Amz-Bypass-Governance-Retention");
        if (bypassgovernance_l != header_list.end() || bypassgovernance_u != header_list.end())
        {
            m_bypassGovernanceRetention = ( bypassgovernance_l != header_list.end() ) ? bypassgovernance_l->value() : bypassgovernance_u->value();
            m_bypassGovernanceRetentionHasBeenSet = true;
        }

        const auto& expec_l = header_list.find("x-amz-expected-bucket-owner");
        const auto& expec_u = header_list.find("X-Amz-Expected-Bucket-Owner");
        if (expec_l != header_list.end() || expec_u != header_list.end())
        {
            m_expectedBucketOwner = ( expec_l != header_list.end() ) ? expec_l->value() : expec_u->value();
            m_expectedBucketOwnerHasBeenSet = true;
        }
        const auto& checksum_l = header_list.find("x-amz-sdk-checksum-algorithm");
        const auto& checksum_u = header_list.find("X-Amz-Sdk-Checksum-Algorithm");
        if (expec_l != header_list.end() || expec_u != header_list.end())
        {
            m_checksumAlgorithm = ( expec_l != header_list.end() ) ? expec_l->value() : expec_u->value();
            m_checksumAlgorithmHasBeenSet = true;
        }

        return *this;
    }

    std::map<std::string, std::string> delete_objects_request::get_request_specific_headers() const
    {
        std::map<std::string, std::string> headers;

        return headers;
    }

} // uh::cluster::http::rest::model
