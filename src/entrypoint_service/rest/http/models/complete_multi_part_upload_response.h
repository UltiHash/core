#pragma once

#include "../http_response.h"
#include "../http_request.h"

namespace uh::cluster::rest::http::model
{

    class complete_multi_part_upload_response : public http_response
    {
    public:
        explicit complete_multi_part_upload_response(const http_request&);
        complete_multi_part_upload_response(const http_request&, http::response<http::string_body>);

        [[nodiscard]] const http::response<http::string_body>& get_response_specific_object() override;
        void set_etag(std::string);
        void set_size(double);
        void set_effective_size(double);
        void set_space_savings(double);
        void set_bandwidth(double);

    private:

        bool m_bandwidthHasBeenSet = false;
        double m_bandwidth;

        bool m_originalSizeHasBeenSet = false;
        double m_originalSize;

        bool m_effectiveSizeHasBeenSet = false;
        double m_effectiveSize;

        bool m_spaceSavingsHasBeenSet = false;
        double m_spaceSavings;

        bool m_locationHasBeenSet = false;
        std::string m_location;

        bool m_bucketHasBeenSet = false;
        std::string m_bucket;

        bool m_keyHasBeenSet = false;
        std::string m_key;

        bool m_expirationHasBeenSet = false;
        std::string m_expiration;

        bool m_eTagHasBeenSet = false;
        std::string m_eTag;

        bool m_checksumCRC32HasBeenSet = false;
        std::string m_checksumCRC32;

        bool m_checksumCRC32CHasBeenSet = false;
        std::string m_checksumCRC32C;

        bool m_checksumSHA1HasBeenSet = false;
        std::string m_checksumSHA1;

        bool m_checksumSHA256HasBeenSet = false;
        std::string m_checksumSHA256;

        bool m_serverSideEncryptionHasBeenSet = false;
        std::string m_serverSideEncryption;

        bool m_versionIdHasBeenSet = false;
        std::string m_versionId;

        bool m_sSEKMSKeyIdHasBeenSet = false;
        std::string m_sSEKMSKeyId;

        bool m_bucketKeyEnabledHasBeenSet = false;
        std::string m_bucketKeyEnabled;

        bool m_requestChargedHasBeenSet = false;
        std::string m_requestCharged;

        bool m_requestIdHasBeenSet = false;
        std::string m_requestId;

    };

} // namespace uh::cluster::rest::http::model
