#pragma once

#include <entry_node/rest/http/http_request.h>
#include <entry_node/rest/utils/time/date_time.h>
#include "object_canned_acl.h"

namespace uh::cluster::rest::http::model
{

    class put_object_request : public http_request
    {
    public:
        explicit put_object_request(const http::request_parser<http::empty_body>&);

        ~put_object_request() override = default;

        [[nodiscard]] inline http_request_type get_request_name() const override { return http_request_type::PUT_OBJECT; }

        [[nodiscard]] std::map<std::string, std::string> get_request_specific_headers() const override;

    private:

        put_object_request& operator = (const http::request_parser<http::empty_body>& recv_req);

        object_canned_acl m_aCL;
        bool m_aCLHasBeenSet = false;

        std::string m_bucket;
        bool m_bucketHasBeenSet = false;

        std::string m_cacheControl;
        bool m_cacheControlHasBeenSet = false;

        std::string m_contentDisposition;
        bool m_contentDispositionHasBeenSet = false;

        std::string m_contentEncoding;
        bool m_contentEncodingHasBeenSet = false;

        std::string m_contentLanguage;
        bool m_contentLanguageHasBeenSet = false;

        long long m_contentLength{};

        bool m_contentLengthHasBeenSet = false;

        std::string m_contentMD5;
        bool m_contentMD5HasBeenSet = false;

        std::string m_contentType;
        bool m_contentTypeHasBeenSet = false;

        std::string m_checksumAlgorithm;
        bool m_checksumAlgorithmHasBeenSet = false;

        std::string m_checksumCRC32;
        bool m_checksumCRC32HasBeenSet = false;

        std::string m_checksumCRC32C;
        bool m_checksumCRC32CHasBeenSet = false;

        std::string m_checksumSHA1;
        bool m_checksumSHA1HasBeenSet = false;

        std::string m_checksumSHA256;
        bool m_checksumSHA256HasBeenSet = false;

        std::string m_expires;

        bool m_expiresHasBeenSet = false;

        std::string m_grantFullControl;
        bool m_grantFullControlHasBeenSet = false;

        std::string m_grantRead;
        bool m_grantReadHasBeenSet = false;

        std::string m_grantReadACP;
        bool m_grantReadACPHasBeenSet = false;

        std::string m_grantWriteACP;
        bool m_grantWriteACPHasBeenSet = false;

        std::string m_key;
        bool m_keyHasBeenSet = false;

        std::map<std::string, std::string> m_metadata;
        bool m_metadataHasBeenSet = false;

        std::string m_serverSideEncryption;
        bool m_serverSideEncryptionHasBeenSet = false;

        std::string m_storageClass;
        bool m_storageClassHasBeenSet = false;

        std::string m_websiteRedirectLocation;
        bool m_websiteRedirectLocationHasBeenSet = false;

        std::string m_sSECustomerAlgorithm;
        bool m_sSECustomerAlgorithmHasBeenSet = false;

        std::string m_sSECustomerKey;
        bool m_sSECustomerKeyHasBeenSet = false;

        std::string m_sSECustomerKeyMD5;
        bool m_sSECustomerKeyMD5HasBeenSet = false;

        std::string m_sSEKMSKeyId;
        bool m_sSEKMSKeyIdHasBeenSet = false;

        std::string m_sSEKMSEncryptionContext;
        bool m_sSEKMSEncryptionContextHasBeenSet = false;

        bool m_bucketKeyEnabled{};

        bool m_bucketKeyEnabledHasBeenSet = false;

        std::string m_requestPayer;
        bool m_requestPayerHasBeenSet = false;

        std::string m_tagging;
        bool m_taggingHasBeenSet = false;

        std::string m_objectLockMode;
        bool m_objectLockModeHasBeenSet = false;

        std::string m_objectLockRetainUntilDate;

        bool m_objectLockRetainUntilDateHasBeenSet = false;

        std::string m_objectLockLegalHoldStatus;
        bool m_objectLockLegalHoldStatusHasBeenSet = false;

        std::string m_expectedBucketOwner;
        bool m_expectedBucketOwnerHasBeenSet = false;

        std::map<std::string, std::string> m_customizedAccessLogTag;
        bool m_customizedAccessLogTagHasBeenSet = false;
    };

} // uh::cluster::rest::http::model
