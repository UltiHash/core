#pragma once

#include <entry_node/rest/http/http_request.h>
#include "bucket_canned_acl.h"

namespace uh::cluster::rest::http::model
{

    class create_bucket_request : public http_request
    {
    public:
        explicit create_bucket_request(const http::request_parser<http::empty_body>&, std::unique_ptr<rest::http::URI>);

        ~create_bucket_request() override = default;

        [[nodiscard]] inline http_request_type get_request_name() const override { return http_request_type::CREATE_BUCKET; }

        [[nodiscard]] std::map<std::string, std::string> get_request_specific_headers() const override;

    private:

        create_bucket_request& operator = (const http::request_parser<http::empty_body>& recv_req);

        bucket_canned_acl m_aCL;
        bool m_aCLHasBeenSet = false;

        std::string m_bucket;
        bool m_bucketHasBeenSet = false;

        std::string m_createBucketConfiguration;
        bool m_createBucketConfigurationHasBeenSet = false;

        std::string m_grantFullControl;
        bool m_grantFullControlHasBeenSet = false;

        std::string m_grantRead;
        bool m_grantReadHasBeenSet = false;

        std::string m_grantReadACP;
        bool m_grantReadACPHasBeenSet = false;

        std::string m_grantWrite;
        bool m_grantWriteHasBeenSet = false;

        std::string m_grantWriteACP;
        bool m_grantWriteACPHasBeenSet = false;

        bool m_objectLockEnabledForBucket;
        bool m_objectLockEnabledForBucketHasBeenSet = false;

        std::string m_objectOwnership;
        bool m_objectOwnershipHasBeenSet = false;

        std::map<std::string, std::string> m_customizedAccessLogTag;
        bool m_customizedAccessLogTagHasBeenSet = false;
    };

} // uh::cluster::rest::http::model
