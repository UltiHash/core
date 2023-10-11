#include "create_bucket_request.h"

namespace uh::cluster::rest::http::model
{

    create_bucket_request::create_bucket_request(const http::request_parser<http::empty_body> & recv_req) : http_request(recv_req)
    {
        // parse and set the received request parameters
        *this = recv_req;
    }

    create_bucket_request& create_bucket_request::operator = (const http::request_parser<http::empty_body>& recv_req)
    {
        const auto& header_list = recv_req.get();

        const auto& acl_l = header_list.find("x-amz-acl");
        const auto& acl_u = header_list.find("X-Amz-Acl");
        if (acl_l != header_list.end() || acl_u != header_list.end())
        {
            m_aCL = ( acl_l != header_list.end() ) ? get_bucket_canned_ACL_for_name(acl_l->value()) : get_bucket_canned_ACL_for_name(acl_u->value());
            m_aCLHasBeenSet = true;
        }

        const auto& grant_full_control_l = header_list.find("x-amz-grant-full-control");
        const auto& grant_full_control_u = header_list.find("X-Amz-Grant-Full-Control");
        if (grant_full_control_l != header_list.end() || grant_full_control_u != header_list.end())
        {
            m_grantFullControl = (grant_full_control_l != header_list.end()) ? grant_full_control_l->value() : grant_full_control_u->value() ;
            m_grantFullControlHasBeenSet = true;
        }

        const auto& grant_read_l = header_list.find("x-amz-grant-read");
        const auto& grant_read_u = header_list.find("X-Amz-Grant-Read");
        if (grant_read_l != header_list.end() || grant_read_u != header_list.end())
        {
            m_grantRead = (grant_read_l != header_list.end()) ? grant_read_l->value() : grant_read_u->value();
            m_grantReadHasBeenSet = true;
        }

        const auto& grant_read_acp_l = header_list.find("x-amz-grant-read-acp");
        const auto& grant_read_acp_u = header_list.find("X-Amz-Grant-Read-Acp");
        if (grant_read_acp_l != header_list.end() || grant_read_acp_u != header_list.end())
        {
            m_grantReadACP = (grant_read_acp_l != header_list.end()) ? grant_read_acp_l->value() : grant_read_acp_u->value();
            m_grantReadACPHasBeenSet = true;
        }

        const auto& grant_write_l = header_list.find("x-amz-grant-write");
        const auto& grant_write_u = header_list.find("X-Amz-Grant-Write");
        if (grant_write_l != header_list.end() || grant_write_u != header_list.end())
        {
            m_grantRead = (grant_write_l != header_list.end()) ? grant_write_l->value() : grant_write_u->value();
            m_grantReadHasBeenSet = true;
        }

        const auto& grant_write_acp_l = header_list.find("x-amz-grant-write-acp");
        const auto& grant_write_acp_u = header_list.find("X-Amz-Grant-Write-Acp");
        if (grant_write_acp_l != header_list.end() || grant_write_acp_u != header_list.end())
        {
            m_grantWriteACP = (grant_write_acp_l != header_list.end()) ? grant_write_acp_l->value() : grant_write_acp_u->value() ;
            m_grantWriteACPHasBeenSet = true;
        }

        const auto& object_lock_enabled_l = header_list.find("x-amz-object-lock-mode");
        const auto& object_lock_enabled_u = header_list.find("X-Amz-Object-Lock-Mode");
        if (object_lock_enabled_l != header_list.end() || object_lock_enabled_u != header_list.end())
        {
            if ( object_lock_enabled_u->value() == "true" || object_lock_enabled_u->value() == "True" ||
                object_lock_enabled_l->value() == "true" || object_lock_enabled_l->value() == "True")
                    m_objectLockEnabledForBucket = true;
                else
                    m_objectLockEnabledForBucket = false;

            m_objectLockEnabledForBucketHasBeenSet = true;
        }

        const auto& object_ownership_l = header_list.find("x-amz-object-ownership");
        const auto& object_ownership_u = header_list.find("X-Amz-Object-Ownership");
        if (object_ownership_l != header_list.end() || object_ownership_u != header_list.end())
        {
            m_objectOwnership = (object_ownership_l != header_list.end()) ? object_ownership_l->value() : object_ownership_u->value() ;
            m_objectOwnershipHasBeenSet = true;
        }

        return *this;
    }

    std::map<std::string, std::string> create_bucket_request::get_request_specific_headers() const
    {
        std::map<std::string, std::string> headers;
        std::stringstream ss;

        if(m_aCLHasBeenSet)
        {
            ss << get_name_for_bucket_canned_ACL(m_aCL);
            headers.emplace("x-amz-acl",  ss.str());
            ss.str("");
        }

        if(m_grantFullControlHasBeenSet)
        {
            ss << m_grantFullControl;
            headers.emplace("x-amz-grant-full-control",  ss.str());
            ss.str("");
        }

        if(m_grantReadHasBeenSet)
        {
            ss << m_grantRead;
            headers.emplace("x-amz-grant-read",  ss.str());
            ss.str("");
        }

        if(m_grantReadACPHasBeenSet)
        {
            ss << m_grantReadACP;
            headers.emplace("x-amz-grant-read-acp",  ss.str());
            ss.str("");
        }

        if(m_grantWriteACPHasBeenSet)
        {
            ss << m_grantWriteACP;
            headers.emplace("x-amz-grant-write-acp",  ss.str());
            ss.str("");
        }

        if(m_objectLockEnabledForBucketHasBeenSet)
        {
            ss << m_objectLockEnabledForBucket;
            headers.emplace("x-amz-bucket-object-lock-enabled",  ss.str());
            ss.str("");
        }

        if(m_objectOwnershipHasBeenSet)
        {
            ss << m_objectOwnership;
            headers.emplace("x-amz-object-ownership",  ss.str());
            ss.str("");
        }

        return headers;
    }

} // uh::cluster::http::rest::model
