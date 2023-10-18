#include "list_multi_part_uploads_response.h"

namespace uh::cluster::rest::http::model
{

    list_multi_part_uploads_response::list_multi_part_uploads_response(const http_request& req) : http_response(req)
    {
        m_res.set(boost::beast::http::field::content_type, "application/xml");
    }

    list_multi_part_uploads_response::list_multi_part_uploads_response(const http_request& req, http::response<http::string_body> recv_res) :
            http_response(req, std::move(recv_res))
    {
        m_res.set(boost::beast::http::field::content_type, "application/xml");
    }

    const http::response<http::string_body>& list_multi_part_uploads_response::get_response_specific_object()
    {

        if(m_errorHasBeenSet)
        {
            m_error.prepare_payload();
            return m_error;
        }

        if(m_requestChargedHasBeenSet)
        {
            m_res.set("x-amz-request-charged", m_requestCharged);
        }

        set_body(std::string("<ListMultipartUploadsResult>\n"
                             "   <Bucket>string</Bucket>\n"
                             "   <KeyMarker>string</KeyMarker>\n"
                             "   <UploadIdMarker>string</UploadIdMarker>\n"
                             "   <NextKeyMarker>string</NextKeyMarker>\n"
                             "   <Prefix>string</Prefix>\n"
                             "   <Delimiter>string</Delimiter>\n"
                             "   <NextUploadIdMarker>string</NextUploadIdMarker>\n"
                             "   <MaxUploads>integer</MaxUploads>\n"
                             "   <IsTruncated>boolean</IsTruncated>\n"
                             "   <Upload>\n"
                             "      <ChecksumAlgorithm>string</ChecksumAlgorithm>\n"
                             "      <Initiated>timestamp</Initiated>\n"
                             "      <Initiator>\n"
                             "         <DisplayName>string</DisplayName>\n"
                             "         <ID>string</ID>\n"
                             "      </Initiator>\n"
                             "      <Key>string</Key>\n"
                             "      <Owner>\n"
                             "         <DisplayName>string</DisplayName>\n"
                             "         <ID>string</ID>\n"
                             "      </Owner>\n"
                             "      <StorageClass>string</StorageClass>\n"
                             "      <UploadId>string</UploadId>\n"
                             "   </Upload>\n"
                             "   <CommonPrefixes>\n"
                             "      <Prefix>string</Prefix>\n"
                             "   </CommonPrefixes>\n"
                             "   <EncodingType>string</EncodingType>\n"
                             "</ListMultipartUploadsResult>"));

        m_res.prepare_payload();
        return m_res;

    }

} // namespace uh::cluster::rest::http::model
