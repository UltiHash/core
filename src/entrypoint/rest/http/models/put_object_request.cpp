#include "put_object_request.h"
#include <iostream>

namespace uh::cluster::rest::http::model {

put_object_request::put_object_request(
    const http::request_parser<http::empty_body>& recv_req,
    std::unique_ptr<rest::http::URI> uri)
    : http_request(recv_req, std::move(uri)) {
    // parse and set the received request parameters
    *this = recv_req;
}

put_object_request& put_object_request::operator=(
    const http::request_parser<http::empty_body>& recv_req) {
    const auto& header_list = recv_req.get();

    const auto& acl_l = header_list.find("x-amz-acl");
    const auto& acl_u = header_list.find("X-Amz-Acl");
    if (acl_l != header_list.end() || acl_u != header_list.end()) {
        m_aCL = (acl_l != header_list.end())
                    ? get_object_canned_acl_for_name(acl_l->value())
                    : get_object_canned_acl_for_name(acl_u->value());
        m_aCLHasBeenSet = true;
    }

    const auto& cache_control =
        header_list.find(boost::beast::http::field::cache_control);
    if (cache_control != header_list.end()) {
        m_cacheControl = cache_control->value();
        m_cacheControlHasBeenSet = true;
    }

    const auto& content_disposition =
        header_list.find(boost::beast::http::field::content_disposition);
    if (content_disposition != header_list.end()) {
        m_contentDisposition = content_disposition->value();
        m_contentDispositionHasBeenSet = true;
    }

    const auto& content_encoding =
        header_list.find(boost::beast::http::field::content_encoding);
    if (content_encoding != header_list.end()) {
        m_contentEncoding = content_encoding->value();
        m_contentEncodingHasBeenSet = true;
    }

    const auto& content_language =
        header_list.find(boost::beast::http::field::content_language);
    if (content_language != header_list.end()) {
        m_contentLanguage = content_language->value();
        m_contentLanguageHasBeenSet = true;
    }

    const auto& content_length =
        header_list.find(boost::beast::http::field::content_length);
    if (content_length != header_list.end()) {
        m_contentLength = std::stoll(content_length->value());
        m_contentLengthHasBeenSet = true;
    }

    const auto& content_md5 =
        header_list.find(boost::beast::http::field::content_md5);
    if (content_md5 != header_list.end()) {
        m_contentMD5 = content_md5->value();
        m_contentMD5HasBeenSet = true;
    }

    const auto& content_type =
        header_list.find(boost::beast::http::field::content_type);
    if (content_type != header_list.end()) {
        m_contentType = content_type->value();
        m_contentTypeHasBeenSet = true;
    }

    const auto& checksum_algorithm_l =
        header_list.find("x-amz-sdk-checksum-algorithm");
    const auto& checksum_algorithm_u =
        header_list.find("X-Amz-Sdk-Checksum-Algorithm");
    if (checksum_algorithm_l != header_list.end() ||
        checksum_algorithm_u != header_list.end()) {
        m_checksumAlgorithm = (checksum_algorithm_l != header_list.end())
                                  ? checksum_algorithm_l->value()
                                  : checksum_algorithm_u->value();
        m_checksumAlgorithmHasBeenSet = true;
    }

    const auto& checksum_crc32_l = header_list.find("x-amz-checksum-crc32");
    const auto& checksum_crc32_u = header_list.find("X-Amz-Checksum-Crc32");
    if (checksum_crc32_l != header_list.end() ||
        checksum_crc32_u != header_list.end()) {
        m_checksumCRC32 = (checksum_crc32_l != header_list.end())
                              ? checksum_crc32_l->value()
                              : checksum_crc32_u->value();
        m_checksumCRC32HasBeenSet = true;
    }

    const auto& checksum_crc32c_l = header_list.find("x-amz-checksum-crc32c");
    const auto& checksum_crc32c_u = header_list.find("X-Amz-Checksum-Crc32c");
    if (checksum_crc32c_l != header_list.end() ||
        checksum_crc32c_u != header_list.end()) {
        m_checksumCRC32C = (checksum_crc32c_l != header_list.end())
                               ? checksum_crc32c_l->value()
                               : checksum_crc32c_u->value();
        m_checksumCRC32CHasBeenSet = true;
    }

    const auto& checksum_sha1_l = header_list.find("x-amz-checksum-sha1");
    const auto& checksum_sha1_u = header_list.find("X-Amz-Checksum-Sha1");
    if (checksum_sha1_l != header_list.end() ||
        checksum_sha1_u != header_list.end()) {
        m_checksumSHA1 = (checksum_sha1_l != header_list.end())
                             ? checksum_sha1_l->value()
                             : checksum_sha1_u->value();
        m_checksumSHA1HasBeenSet = true;
    }

    const auto& checksum_sha256_l = header_list.find("x-amz-checksum-sha256");
    const auto& checksum_sha256_u = header_list.find("X-Amz-Checksum-Sha256");
    if (checksum_sha256_l != header_list.end() ||
        checksum_sha256_u != header_list.end()) {
        m_checksumSHA256 = (checksum_sha256_l != header_list.end())
                               ? checksum_sha256_l->value()
                               : checksum_sha256_u->value();
        m_checksumSHA256HasBeenSet = true;
    }

    const auto& expires = header_list.find(boost::beast::http::field::expires);
    if (expires != header_list.end()) {
        m_expires = expires->value();
        m_expiresHasBeenSet = true;
    }

    const auto& grant_full_control_l =
        header_list.find("x-amz-grant-full-control");
    const auto& grant_full_control_u =
        header_list.find("X-Amz-Grant-Full-Control");
    if (grant_full_control_l != header_list.end() ||
        grant_full_control_u != header_list.end()) {
        m_grantFullControl = (grant_full_control_l != header_list.end())
                                 ? grant_full_control_l->value()
                                 : grant_full_control_u->value();
        m_grantFullControlHasBeenSet = true;
    }

    const auto& grant_read_l = header_list.find("x-amz-grant-read");
    const auto& grant_read_u = header_list.find("X-Amz-Grant-Read");
    if (grant_read_l != header_list.end() ||
        grant_read_u != header_list.end()) {
        m_grantRead = (grant_read_l != header_list.end())
                          ? grant_read_l->value()
                          : (grant_read_u->value());
        m_grantReadHasBeenSet = true;
    }

    const auto& grant_read_acp_l = header_list.find("x-amz-grant-read-acp");
    const auto& grant_read_acp_u = header_list.find("X-Amz-Grant-Read-Acp");
    if (grant_read_acp_l != header_list.end() ||
        grant_read_acp_u != header_list.end()) {
        m_grantReadACP = (grant_read_acp_l != header_list.end())
                             ? grant_read_acp_l->value()
                             : grant_read_acp_u->value();
        m_grantReadACPHasBeenSet = true;
    }

    const auto& grant_write_acp_l = header_list.find("x-amz-grant-write-acp");
    const auto& grant_write_acp_u = header_list.find("X-Amz-Grant-Write-Acp");
    if (grant_write_acp_l != header_list.end() ||
        grant_write_acp_u != header_list.end()) {
        m_grantWriteACP = (grant_write_acp_l != header_list.end())
                              ? grant_write_acp_l->value()
                              : grant_write_acp_u->value();
        m_grantWriteACPHasBeenSet = true;
    }

    const auto& server_side_encryption_l =
        header_list.find("x-amz-server-side-encryption");
    const auto& server_side_encryption_u =
        header_list.find("X-Amz-Server-Side-Encryption");
    if (server_side_encryption_l != header_list.end() ||
        server_side_encryption_u != header_list.end()) {
        m_serverSideEncryption = (server_side_encryption_l != header_list.end())
                                     ? server_side_encryption_l->value()
                                     : server_side_encryption_u->value();
        m_serverSideEncryptionHasBeenSet = true;
    }

    const auto& storage_class_l = header_list.find("x-amz-storage-class");
    const auto& storage_class_u = header_list.find("X-Amz-Storage-Class");
    if (storage_class_l != header_list.end() ||
        storage_class_u != header_list.end()) {
        m_storageClass = (storage_class_l != header_list.end())
                             ? storage_class_l->value()
                             : storage_class_u->value();
        m_storageClassHasBeenSet = true;
    }

    const auto& website_redirect_location_l =
        header_list.find("x-amz-website-redirect-location");
    const auto& website_redirect_location_u =
        header_list.find("X-Amz-Website-Redirect-Location");
    if (website_redirect_location_l != header_list.end() ||
        website_redirect_location_u != header_list.end()) {
        m_websiteRedirectLocation =
            (website_redirect_location_l != header_list.end())
                ? website_redirect_location_l->value()
                : website_redirect_location_u->value();
        m_websiteRedirectLocationHasBeenSet = true;
    }

    const auto& sse_customer_algorithm_l =
        header_list.find("x-amz-server-side-encryption-customer-algorithm");
    const auto& sse_customer_algorithm_u =
        header_list.find("X-Amz-Server-Side-Encryption-Customer-Algorithm");
    if (sse_customer_algorithm_l != header_list.end() ||
        sse_customer_algorithm_u != header_list.end()) {
        m_sSECustomerAlgorithm = (sse_customer_algorithm_l != header_list.end())
                                     ? sse_customer_algorithm_l->value()
                                     : sse_customer_algorithm_u->value();
        m_sSECustomerAlgorithmHasBeenSet = true;
    }

    const auto& sse_customer_key_l =
        header_list.find("x-amz-server-side-encryption-customer-key");
    const auto& sse_customer_key_u =
        header_list.find("X-Amz-Server-Side-Encryption-Customer-Key");
    if (sse_customer_key_l != header_list.end() ||
        sse_customer_key_u != header_list.end()) {
        m_sSECustomerKey = (sse_customer_key_l != header_list.end())
                               ? sse_customer_key_l->value()
                               : sse_customer_key_u->value();
        m_sSECustomerKeyHasBeenSet = true;
    }

    const auto& sse_customer_key_md5_l =
        header_list.find("x-amz-server-side-encryption-customer-key-MD5");
    const auto& sse_customer_key_md5_u =
        header_list.find("X-Amz-Server-Side-Encryption-Customer-Key-MD5");
    if (sse_customer_key_md5_l != header_list.end() ||
        sse_customer_key_md5_u != header_list.end()) {
        m_sSECustomerKeyMD5 = (sse_customer_key_md5_l != header_list.end())
                                  ? sse_customer_key_md5_l->value()
                                  : sse_customer_key_md5_u->value();
        m_sSECustomerKeyMD5HasBeenSet = true;
    }

    const auto& sse_aws_kms_key_id_l =
        header_list.find("x-amz-server-side-encryption-aws-kms-key-id");
    const auto& sse_aws_kms_key_id_u =
        header_list.find("X-Amz-Server-Side-Encryption-Aws-Kms-Key-Id");
    if (sse_aws_kms_key_id_l != header_list.end() ||
        sse_aws_kms_key_id_u != header_list.end()) {
        m_sSEKMSKeyId = (sse_aws_kms_key_id_l != header_list.end())
                            ? sse_aws_kms_key_id_l->value()
                            : sse_aws_kms_key_id_u->value();
        m_sSEKMSKeyIdHasBeenSet = true;
    }

    const auto& sse_context_l =
        header_list.find("x-amz-server-side-encryption-context");
    const auto& sse_context_u =
        header_list.find("X-Amz-Server-Side-Encryption-Context");
    if (sse_context_l != header_list.end() ||
        sse_context_u != header_list.end()) {
        m_sSEKMSEncryptionContext = (sse_context_l != header_list.end())
                                        ? sse_context_l->value()
                                        : sse_context_u->value();
        m_sSEKMSEncryptionContextHasBeenSet = true;
    }

    const auto& sse_bucket_key_enabled_l =
        header_list.find("x-amz-server-side-encryption-bucket-key-enabled");
    const auto& sse_bucket_key_enabled_u =
        header_list.find("X-Amz-Server-Side-Encryption-Bucket-Key-Enabled");
    if (sse_bucket_key_enabled_l != header_list.end() ||
        sse_bucket_key_enabled_u != header_list.end()) {
        m_bucketKeyEnabled = (sse_bucket_key_enabled_l != header_list.end())
                                 ? std::stoi(sse_bucket_key_enabled_l->value())
                                 : std::stoi(sse_bucket_key_enabled_u->value());
        m_bucketKeyEnabledHasBeenSet = true;
    }

    const auto& request_payer_l = header_list.find("x-amz-request-payer");
    const auto& request_payer_u = header_list.find("X-Amz-Request-Payer");
    if (request_payer_l != header_list.end() ||
        request_payer_u != header_list.end()) {
        m_requestPayer = (request_payer_l != header_list.end())
                             ? request_payer_l->value()
                             : request_payer_u->value();
        m_requestPayerHasBeenSet = true;
    }

    const auto& tagging_l = header_list.find("x-amz-tagging");
    const auto& tagging_u = header_list.find("X-Amz-Tagging");
    if (tagging_l != header_list.end() || tagging_u != header_list.end()) {
        m_tagging = (tagging_l != header_list.end()) ? tagging_l->value()
                                                     : tagging_u->value();
        m_taggingHasBeenSet = true;
    }

    const auto& object_lock_mode_l = header_list.find("x-amz-object-lock-mode");
    const auto& object_lock_mode_u = header_list.find("X-Amz-Object-Lock-Mode");
    if (object_lock_mode_l != header_list.end() ||
        object_lock_mode_u != header_list.end()) {
        m_objectLockMode = (object_lock_mode_l != header_list.end())
                               ? object_lock_mode_l->value()
                               : object_lock_mode_u->value();
        m_objectLockModeHasBeenSet = true;
    }

    const auto& object_lock_retain_until_date_l =
        header_list.find("x-amz-object-lock-retain-until-date");
    const auto& object_lock_retain_until_date_u =
        header_list.find("X-Amz-Object-Lock-Retain-Until-Date");
    if (object_lock_retain_until_date_l != header_list.end() ||
        object_lock_retain_until_date_u != header_list.end()) {
        m_objectLockRetainUntilDate =
            (object_lock_retain_until_date_l != header_list.end())
                ? object_lock_retain_until_date_l->value()
                : object_lock_retain_until_date_u->value();
        m_objectLockRetainUntilDateHasBeenSet = true;
    }

    const auto& object_lock_legal_hold_l =
        header_list.find("x-amz-object-lock-legal-hold");
    const auto& object_lock_legal_hold_u =
        header_list.find("X-Amz-Object-Lock-Legal-Hold");
    if (object_lock_legal_hold_l != header_list.end() ||
        object_lock_legal_hold_u != header_list.end()) {
        m_objectLockLegalHoldStatus =
            (object_lock_legal_hold_l != header_list.end())
                ? object_lock_legal_hold_l->value()
                : object_lock_legal_hold_u->value();
        m_objectLockLegalHoldStatusHasBeenSet = true;
    }

    const auto& expected_bucket_owner_l =
        header_list.find("x-amz-expected-bucket-owner");
    const auto& expected_bucket_owner_u =
        header_list.find("X-Amz-Expected-Bucket-Owner");
    if (expected_bucket_owner_l != header_list.end() ||
        expected_bucket_owner_u != header_list.end()) {
        m_expectedBucketOwner = (expected_bucket_owner_l != header_list.end())
                                    ? expected_bucket_owner_l->value()
                                    : expected_bucket_owner_u->value();
        m_expectedBucketOwnerHasBeenSet = true;
    }

    return *this;
}

std::map<std::string, std::string>
put_object_request::get_request_specific_headers() const {
    std::map<std::string, std::string> headers;
    std::stringstream ss;

    if (m_aCLHasBeenSet) {
        headers.emplace("x-amz-acl", get_name_for_object_canned_acl(m_aCL));
    }

    if (m_cacheControlHasBeenSet) {
        ss << m_cacheControl;
        headers.emplace("cache-control", ss.str());
        ss.str("");
    }

    if (m_contentDispositionHasBeenSet) {
        ss << m_contentDisposition;
        headers.emplace("content-disposition", ss.str());
        ss.str("");
    }

    if (m_contentEncodingHasBeenSet) {
        ss << m_contentEncoding;
        headers.emplace("content-encoding", ss.str());
        ss.str("");
    }

    if (m_contentLanguageHasBeenSet) {
        ss << m_contentLanguage;
        headers.emplace("content-language", ss.str());
        ss.str("");
    }

    if (m_contentLengthHasBeenSet) {
        ss << m_contentLength;
        headers.emplace("content-length", ss.str());
        ss.str("");
    }

    if (m_contentMD5HasBeenSet) {
        ss << m_contentMD5;
        headers.emplace("content-md5", ss.str());
        ss.str("");
    }

    if (m_contentTypeHasBeenSet) {
        ss << m_contentType;
        headers.emplace("content-type", ss.str());
        ss.str("");
    }

    if (m_checksumAlgorithmHasBeenSet) {
        ss << m_checksumAlgorithm;
        headers.emplace("x-amz-sdk-checksum-algorithm", ss.str());
        ss.str("");
    }

    if (m_checksumCRC32HasBeenSet) {
        ss << m_checksumCRC32;
        headers.emplace("x-amz-checksum-crc32", ss.str());
        ss.str("");
    }

    if (m_checksumCRC32CHasBeenSet) {
        ss << m_checksumCRC32C;
        headers.emplace("x-amz-checksum-crc32c", ss.str());
        ss.str("");
    }

    if (m_checksumSHA1HasBeenSet) {
        ss << m_checksumSHA1;
        headers.emplace("x-amz-checksum-sha1", ss.str());
        ss.str("");
    }

    if (m_checksumSHA256HasBeenSet) {
        ss << m_checksumSHA256;
        headers.emplace("x-amz-checksum-sha256", ss.str());
        ss.str("");
    }

    if (m_expiresHasBeenSet) {
        ss << m_expires;
        headers.emplace("expires", ss.str());
        ss.str("");
    }

    if (m_grantFullControlHasBeenSet) {
        ss << m_grantFullControl;
        headers.emplace("x-amz-grant-full-control", ss.str());
        ss.str("");
    }

    if (m_grantReadHasBeenSet) {
        ss << m_grantRead;
        headers.emplace("x-amz-grant-read", ss.str());
        ss.str("");
    }

    if (m_grantReadACPHasBeenSet) {
        ss << m_grantReadACP;
        headers.emplace("x-amz-grant-read-acp", ss.str());
        ss.str("");
    }

    if (m_grantWriteACPHasBeenSet) {
        ss << m_grantWriteACP;
        headers.emplace("x-amz-grant-write-acp", ss.str());
        ss.str("");
    }

    if (m_metadataHasBeenSet) {
        for (const auto& item : m_metadata) {
            ss << "x-amz-meta-" << item.first;
            headers.emplace(ss.str(), item.second);
            ss.str("");
        }
    }

    if (m_serverSideEncryptionHasBeenSet) {
        ss << m_serverSideEncryption;
        headers.emplace("x-amz-server-side-encryption", ss.str());
        ss.str("");
    }

    if (m_storageClassHasBeenSet) {
        ss << m_storageClass;
        headers.emplace("x-amz-storage-class", ss.str());
        ss.str("");
    }

    if (m_websiteRedirectLocationHasBeenSet) {
        ss << m_websiteRedirectLocation;
        headers.emplace("x-amz-website-redirect-location", ss.str());
        ss.str("");
    }

    if (m_sSECustomerAlgorithmHasBeenSet) {
        ss << m_sSECustomerAlgorithm;
        headers.emplace("x-amz-server-side-encryption-customer-algorithm",
                        ss.str());
        ss.str("");
    }

    if (m_sSECustomerKeyHasBeenSet) {
        ss << m_sSECustomerKey;
        headers.emplace("x-amz-server-side-encryption-customer-key", ss.str());
        ss.str("");
    }

    if (m_sSECustomerKeyMD5HasBeenSet) {
        ss << m_sSECustomerKeyMD5;
        headers.emplace("x-amz-server-side-encryption-customer-key-md5",
                        ss.str());
        ss.str("");
    }

    if (m_sSEKMSKeyIdHasBeenSet) {
        ss << m_sSEKMSKeyId;
        headers.emplace("x-amz-server-side-encryption-aws-kms-key-id",
                        ss.str());
        ss.str("");
    }

    if (m_sSEKMSEncryptionContextHasBeenSet) {
        ss << m_sSEKMSEncryptionContext;
        headers.emplace("x-amz-server-side-encryption-context", ss.str());
        ss.str("");
    }

    if (m_bucketKeyEnabledHasBeenSet) {
        ss << std::boolalpha << m_bucketKeyEnabled;
        headers.emplace("x-amz-server-side-encryption-bucket-key-enabled",
                        ss.str());
        ss.str("");
    }

    if (m_requestPayerHasBeenSet) {
        ss << m_requestPayer;
        headers.emplace("x-amz-request-payer", ss.str());
        ss.str("");
    }

    if (m_taggingHasBeenSet) {
        ss << m_tagging;
        headers.emplace("x-amz-tagging", ss.str());
        ss.str("");
    }

    if (m_objectLockModeHasBeenSet) {
        ss << m_objectLockMode;
        headers.emplace("x-amz-object-lock-mode", ss.str());
        ss.str("");
    }

    if (m_objectLockRetainUntilDateHasBeenSet) {
        ss << m_objectLockRetainUntilDate;
        headers.emplace("x-amz-object-lock-retain-until-date", ss.str());
        ss.str("");
    }

    if (m_objectLockLegalHoldStatusHasBeenSet) {
        ss << m_objectLockLegalHoldStatus;
        headers.emplace("x-amz-object-lock-legal-hold", ss.str());
        ss.str("");
    }

    if (m_expectedBucketOwnerHasBeenSet) {
        ss << m_expectedBucketOwner;
        headers.emplace("x-amz-expected-bucket-owner", ss.str());
        ss.str("");
    }

    return headers;
}

} // namespace uh::cluster::rest::http::model
