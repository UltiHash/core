#include "multi_part_upload_response.h"

namespace uh::cluster::rest::http::model
{

    multi_part_upload_response::multi_part_upload_response(const http_request& req) : http_response(req)
    {}

    multi_part_upload_response::multi_part_upload_response(const http_request& req, http::response<http::string_body> recv_res) :
            http_response(req, std::move(recv_res))
    {}

    void multi_part_upload_response::set_etag(std::string etag)
    {
        m_eTagHasBeenSet = true;
        m_eTag = std::move(etag);
    }

    const http::response<http::string_body>& multi_part_upload_response::get_response_specific_object()
    {

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

        if(m_serverSideEncryptionHasBeenSet)
        {
            m_res.set("x-amz-server-side-encryption", m_serverSideEncryption);
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

        if(m_requestChargedHasBeenSet)
        {
            m_res.set("x-amz-request-charged", m_requestCharged);
        }

        m_res.prepare_payload();
        return m_res;

    }

} // namespace uh::cluster::rest::http::model
