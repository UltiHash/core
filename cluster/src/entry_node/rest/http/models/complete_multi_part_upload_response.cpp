#include "complete_multi_part_upload_response.h"

namespace uh::cluster::rest::http::model
{

    complete_multi_part_upload_response::complete_multi_part_upload_response(const http_request& req) : http_response(req)
    {
        m_res.set(boost::beast::http::field::content_type, "application/xml");
    }

    complete_multi_part_upload_response::complete_multi_part_upload_response(const http_request& req, http::response<http::string_body> recv_res) :
            http_response(req, std::move(recv_res))
    {
        m_res.set(boost::beast::http::field::content_type, "application/xml");
    }

    void complete_multi_part_upload_response::set_etag(std::string etag)
    {
        m_eTagHasBeenSet = true;
        m_eTag = std::move(etag);
    }

    const http::response<http::string_body>& complete_multi_part_upload_response::get_response_specific_object()
    {

        if(m_errorHasBeenSet)
        {
            m_error.prepare_payload();
            return m_error;
        }

        if(m_expirationHasBeenSet)
        {
            m_res.set("x-amz-expiration", m_expiration);
        }

        if(m_serverSideEncryptionHasBeenSet)
        {
            m_res.set("x-amz-server-side-encryption", m_serverSideEncryption);
        }

        if(m_versionIdHasBeenSet)
        {
            m_res.set("x-amz-version-id", m_versionId);
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

        // xml body
        set_body(std::string("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                             "<CompleteMultipartUploadResult>\n"
                             "<Location>string</Location>\n"
                             "<Bucket>" + m_orig_req.get_URI().get_bucket_id() +"</Bucket>\n"
                             "<Key>" + m_orig_req.get_URI().get_object_key() + "</Key>\n"
                             "<ETag>" + ((m_eTagHasBeenSet) ? m_eTag : "CustomEtag" ) + "</ETag>\n"
                             "</CompleteMultipartUploadResult>"));

        m_res.prepare_payload();
        return m_res;

    }

} // namespace uh::cluster::rest::http::model
