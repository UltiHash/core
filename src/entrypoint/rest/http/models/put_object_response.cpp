#include "put_object_response.h"

namespace uh::cluster::rest::http::model {

put_object_response::put_object_response(const http_request& req)
    : http_response(req) {}

void put_object_response::set_size(std::size_t size) {
    m_originalSize = size;
    m_originalSizeHasBeenSet = true;
}

void put_object_response::set_effective_size(std::size_t effective_size) {
    m_effectiveSize = effective_size;
    m_effectiveSizeHasBeenSet = true;
}

void put_object_response::set_space_savings(double space_savings) {
    m_spaceSavings = space_savings;
    m_spaceSavingsHasBeenSet = true;
}

void put_object_response::set_bandwidth(double bandwidth) {
    m_bandwidth = bandwidth;
    m_bandwidthHasBeenSet = true;
}

const http::response<http::string_body>&
put_object_response::get_response_specific_object() {

    m_res.set(boost::beast::http::field::etag, m_orig_req.get_eTag());

    if (m_expirationHasBeenSet) {
        m_res.set("x-amz-expiration", m_expiration);
    }

    if (m_checksumCRC32HasBeenSet) {
        m_res.set("x-amz-checksum-crc32", m_checksumCRC32);
    }

    if (m_checksumCRC32CHasBeenSet) {
        m_res.set("x-amz-checksum-crc32c", m_checksumCRC32C);
    }

    if (m_checksumSHA1HasBeenSet) {
        m_res.set("x-amz-checksum-sha1", m_checksumSHA1);
    }

    if (m_checksumSHA256HasBeenSet) {
        m_res.set("x-amz-checksum-sha256", m_checksumSHA256);
    }

    if (m_serverSideEncryptionHasBeenSet) {
        m_res.set("x-amz-server-side-encryption", m_serverSideEncryption);
    }

    if (m_versionIdHasBeenSet) {
        m_res.set("x-amz-version-id", m_versionId);
    }

    if (m_sSECustomerAlgorithmHasBeenSet) {
        m_res.set("x-amz-server-side-encryption-customer-algorithm",
                  m_sSECustomerAlgorithm);
    }

    if (m_sSECustomerKeyMD5HasBeenSet) {
        m_res.set("x-amz-server-side-encryption-customer-key-md5",
                  m_sSECustomerKeyMD5);
    }

    if (m_sSEKMSKeyIdHasBeenSet) {
        m_res.set("x-amz-server-side-encryption-aws-kms-key-id", m_sSEKMSKeyId);
    }

    if (m_sSEKMSEncryptionContextHasBeenSet) {
        m_res.set("x-amz-server-side-encryption-context",
                  m_sSEKMSEncryptionContext);
    }

    if (m_bucketKeyEnabledHasBeenSet) {
        m_res.set("x-amz-server-side-encryption-bucket-key-enabled",
                  m_bucketKeyEnabled);
    }

    if (m_requestChargedHasBeenSet) {
        m_res.set("x-amz-request-charged", m_requestCharged);
    }

    if (m_requestIdHasBeenSet) {
        m_res.set("x-amz-request-id", m_requestId);
    }

    if (m_bandwidthHasBeenSet) {
        m_res.set("uh-bandwidth-mbps", std::to_string(m_bandwidth));
    }

    if (m_originalSizeHasBeenSet) {
        m_res.set("uh-original-size", std::to_string(m_originalSize));
        m_res.set("uh-original-size-mb",
                  std::to_string(static_cast<double>(m_originalSize) /
                                 (1024 * 1024)));
    }

    if (m_effectiveSizeHasBeenSet) {
        m_res.set("uh-effective-size", std::to_string(m_effectiveSize));
        m_res.set("uh-effective-size-mb",
                  std::to_string(static_cast<double>(m_effectiveSize) /
                                 (1024 * 1024)));
    }

    if (m_spaceSavingsHasBeenSet) {
        m_res.set("uh-space-savings-ratio", std::to_string(m_spaceSavings));
    }

    m_res.prepare_payload();
    return m_res;
}

} // namespace uh::cluster::rest::http::model
