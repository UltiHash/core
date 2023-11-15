#include "get_object_response.h"

namespace uh::cluster::rest::http::model
{

    get_object_response::get_object_response(const http_request& req) : http_response(req)
    {}

    get_object_response::get_object_response(const http_request& req, http::response<http::string_body> recv_res) :
            http_response(req, std::move(recv_res))
    {}

    void get_object_response::set_bandwidth(double bandwidth)
    {
        m_bandwidth = bandwidth;
        m_bandwidthHasBeenSet = true;
    }

    const http::response<http::string_body>& get_object_response::get_response_specific_object()
    {

        if(m_errorHasBeenSet)
        {
            m_error.prepare_payload();
            return m_error;
        }

        if(m_deleteMarkerHasBeenSet)
        {
            m_res.set("x-amz-delete-marker", m_deleteMarker);
        }

        if(m_acceptRangesHasBeenSet)
        {
            m_res.set("accept-ranges", m_acceptRanges);
        }

        if(m_expirationHasBeenSet)
        {
            m_res.set("x-amz-expiration", m_expiration);
        }

        if(m_restoreHasBeenSet)
        {
            m_res.set("x-amz-restore", m_restore);
        }

        if(m_lastModifiedHasBeenSet)
        {
            m_res.set("last-modified", m_lastModified);
        }

        if(m_contentLengthHasBeenSet)
        {
            m_res.set(boost::beast::http::field::content_length, m_contentLength);
        }

        if(m_eTagHasBeenSet)
        {
            m_res.set(boost::beast::http::field::etag, m_eTag);
        }

        if(m_checksumCRC32HasBeenSet)
        {
            m_res.set("x-amz-checksum-crc32", m_checksumCRC32);
        }

        if(m_checksumCRC32CHasBeenSet)
        {
            m_res.set("x-amz-checksum-crc32c", m_checksumCRC32C);
        }

        if(m_checksumSHA1HasBeenSet)
        {
            m_res.set("x-amz-checksum-sha1", m_checksumSHA1);
        }

        if(m_checksumSHA256HasBeenSet)
        {
            m_res.set("x-amz-checksum-sha256", m_checksumSHA256);
        }

        if(m_missingMetaHasBeenSet)
        {
            m_res.set("x-amz-missing-meta", m_missingMeta);
        }

        if(m_versionIdHasBeenSet)
        {
            m_res.set("x-amz-version-id", m_versionId);
        }

        if(m_cacheControlHasBeenSet)
        {
            m_res.set(boost::beast::http::field::cache_control, m_cacheControl);
        }


        if(m_contentDispositionHasBeenSet)
        {
            m_res.set(boost::beast::http::field::content_disposition, m_contentDisposition);
        }

        if(m_contentEncodingHasBeenSet)
        {
            m_res.set(boost::beast::http::field::encoding, m_contentEncoding);
        }

        if(m_contentLanguageHasBeenSet)
        {
            m_res.set(boost::beast::http::field::language, m_contentLanguage);
        }

        if(m_contentRangeHasBeenSet)
        {
            m_res.set(boost::beast::http::field::range, m_contentRange);
        }

        if(m_contentTypeHasBeenSet)
        {
            m_res.set(boost::beast::http::field::content_type, m_contentType);
        }

        if(m_expiresHasBeenSet)
        {
            m_res.set(boost::beast::http::field::expires, m_expires);
        }

        if(m_websiteRedirectLocationHasBeenSet)
        {
            m_res.set("x-amz-website-redirect-location", m_websiteRedirectLocation);
        }

        if(m_serverSideEncryptionHasBeenSet)
        {
            m_res.set("x-amz-server-side-encryption", m_serverSideEncryption);
        }

        if(m_metadataHasBeenSet)
        {

        }

        if(m_sSECustomerAlgorithmHasBeenSet)
        {
            m_res.set("x-amz-server-side-encryption-customer-algorithm", m_sSECustomerAlgorithm);
        }

        if(m_sSECustomerKeyMD5HasBeenSet)
        {
            m_res.set("x-amz-server-side-encryption-customer-key-md5", m_sSECustomerKeyMD5);
        }

        if(m_sSEKMSKeyIdHasBeenSet)
        {
            m_res.set("x-amz-server-side-encryption-aws-kms-key-id", m_sSEKMSKeyId);
        }

        if(m_bucketKeyEnabledHasBeenSet)
        {
            m_res.set("x-amz-server-side-encryption-bucket-key-enabled", m_bucketKeyEnabled);
        }


        if(m_storageClassHasBeenSet)
        {
            m_res.set("x-amz-storage-class", m_storageClass);
        }

        if(m_requestChargedHasBeenSet)
        {
            m_res.set("x-amz-request-charged", m_requestCharged);
        }

        if(m_replicationStatusHasBeenSet)
        {
            m_res.set("x-amz-replication-status", m_replicationStatus);
        }

        if(m_partsCountHasBeenSet)
        {
            m_res.set("x-amz-mp-parts-count", m_partsCount);
        }

        if(m_m_tagCountHasBeenSet)
        {
            m_res.set("x-amz-tagging-count", m_tagCount);
        }

        if(m_objectLockModeHasBeenSet)
        {
            m_res.set("x-amz-object-lock-mode", m_objectLockMode);
        }

        if(m_objectLockRetainUntilDateHasBeenSet)
        {
            m_res.set("x-amz-object-lock-retain-until-date", m_objectLockRetainUntilDate);
        }

        if(m_objectLockLegalHoldStatusHasBeenSet)
        {
            m_res.set("x-amz-object-lock-legal-hold", m_objectLockLegalHoldStatus);
        }

        if(m_id2HasBeenSet)
        {
            m_res.set("x-amz-id-2", m_id2);
        }

        if(m_requestIdHasBeenSet)
        {
            m_res.set("x-amz-request-id", m_requestId);
        }

        if (m_bandwidthHasBeenSet)
        {
            m_res.set("uh-bandwidth-mbps", std::to_string(m_bandwidth));
        }

        m_res.prepare_payload();
        return m_res;

    }

} // namespace uh::cluster::rest::http::model
