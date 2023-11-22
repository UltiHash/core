#pragma once

#include "entry_node/rest/http/http_response.h"
#include "entry_node/rest/http/http_request.h"

namespace uh::cluster::rest::http::model
{

    class get_object_response : public http_response
    {
    public:
        explicit get_object_response(const http_request&);
        get_object_response(const http_request&, http::response<http::string_body>);

        [[nodiscard]] const http::response<http::string_body>& get_response_specific_object() override;

        void set_bandwidth(double);

    private:

        bool m_bandwidthHasBeenSet = false;
        double m_bandwidth;

        bool m_deleteMarkerHasBeenSet = false;
        std::string m_deleteMarker;

        bool m_acceptRangesHasBeenSet = false;
        std::string m_acceptRanges;

        bool m_expirationHasBeenSet = false;
        std::string m_expiration;

        bool m_restoreHasBeenSet = false;
        std::string m_restore;

        bool m_lastModifiedHasBeenSet = false;
        std::string m_lastModified;

        bool m_contentLengthHasBeenSet = false;
        std::string m_contentLength;

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

        bool m_missingMetaHasBeenSet = false;
        std::string m_missingMeta;

        bool m_versionIdHasBeenSet = false;
        std::string m_versionId;

        bool m_cacheControlHasBeenSet = false;
        std::string m_cacheControl;

        bool m_contentDispositionHasBeenSet = false;
        std::string m_contentDisposition;

        bool m_contentEncodingHasBeenSet = false;
        std::string m_contentEncoding;

        bool m_contentLanguageHasBeenSet = false;
        std::string m_contentLanguage;

        bool m_contentRangeHasBeenSet = false;
        std::string m_contentRange;

        bool m_contentTypeHasBeenSet = false;
        std::string m_contentType;

        bool m_expiresHasBeenSet = false;
        std::string m_expires;

        bool m_websiteRedirectLocationHasBeenSet = false;
        std::string m_websiteRedirectLocation;

        bool m_serverSideEncryptionHasBeenSet = false;
        std::string m_serverSideEncryption;

        bool m_metadataHasBeenSet = false;
        std::map<std::string, std::string> m_metadata;

        bool m_sSECustomerAlgorithmHasBeenSet = false;
        std::string m_sSECustomerAlgorithm;

        bool m_sSECustomerKeyMD5HasBeenSet = false;
        std::string m_sSECustomerKeyMD5;

        bool m_sSEKMSKeyIdHasBeenSet = false;
        std::string m_sSEKMSKeyId;

        bool m_bucketKeyEnabledHasBeenSet = false;
        std::string m_bucketKeyEnabled;

        bool m_storageClassHasBeenSet = false;
        std::string m_storageClass;

        bool m_requestChargedHasBeenSet = false;
        std::string m_requestCharged;

        bool m_replicationStatusHasBeenSet = false;
        std::string m_replicationStatus;

        bool m_partsCountHasBeenSet = false;
        std::string m_partsCount;

        bool m_m_tagCountHasBeenSet = false;
        std::string m_tagCount;

        bool m_objectLockModeHasBeenSet = false;
        std::string m_objectLockMode;

        bool m_objectLockRetainUntilDateHasBeenSet = false;
        std::string m_objectLockRetainUntilDate;

        bool m_objectLockLegalHoldStatusHasBeenSet = false;
        std::string m_objectLockLegalHoldStatus;

        bool m_id2HasBeenSet = false;
        std::string m_id2;

        bool m_requestIdHasBeenSet = false;
        std::string m_requestId;

    };

} // namespace uh::cluster::rest::http::model
