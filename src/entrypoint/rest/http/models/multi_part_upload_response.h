#pragma once

#include "entrypoint/rest/http/http_response.h"
#include "entrypoint/rest/http/http_request.h"

namespace uh::cluster::rest::http::model
{

    class multi_part_upload_response : public http_response
    {
    public:
        explicit multi_part_upload_response(const http_request&);

        [[nodiscard]] const http::response<http::string_body>& get_response_specific_object() override;
    private:

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

        bool m_sSECustomerAlgorithmHasBeenSet = false;
        std::string m_sSECustomerAlgorithm;

        bool m_sSECustomerKeyMD5HasBeenSet = false;
        std::string m_sSECustomerKeyMD5;

        bool m_sSEKMSKeyIdHasBeenSet = false;
        std::string m_sSEKMSKeyId;

        bool m_bucketKeyEnabledHasBeenSet = false;
        std::string m_bucketKeyEnabled;

        bool m_requestChargedHasBeenSet = false;
        std::string m_requestCharged;

    };

} // namespace uh::cluster::rest::http::model
