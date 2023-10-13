#include "list_objectsv2_response.h"

namespace uh::cluster::rest::http::model
{

    list_objectsv2_response::list_objectsv2_response(const http_request& req) : http_response(req)
    {

    }

    list_objectsv2_response::list_objectsv2_response(const http_request& req, http::response<http::string_body> recv_res) :
            http_response(req, std::move(recv_res))
    {}

    const http::response<http::string_body>& list_objectsv2_response::get_response_specific_object()
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

        set_body(std::string("<ListBucketResult>\n"
                             "   <IsTruncated>boolean</IsTruncated>\n"
                             "   <Contents>\n"
                             "      <ChecksumAlgorithm>string</ChecksumAlgorithm>\n"
                             "      <ETag>string</ETag>\n"
                             "      <Key>string</Key>\n"
                             "      <LastModified>timestamp</LastModified>\n"
                             "      <Owner>\n"
                             "         <DisplayName>string</DisplayName>\n"
                             "         <ID>string</ID>\n"
                             "      </Owner>\n"
                             "      <RestoreStatus>\n"
                             "         <IsRestoreInProgress>boolean</IsRestoreInProgress>\n"
                             "         <RestoreExpiryDate>timestamp</RestoreExpiryDate>\n"
                             "      </RestoreStatus>\n"
                             "      <Size>long</Size>\n"
                             "      <StorageClass>string</StorageClass>\n"
                             "   </Contents>\n"
                             "   <Name>string</Name>\n"
                             "   <Prefix>string</Prefix>\n"
                             "   <Delimiter>string</Delimiter>\n"
                             "   <MaxKeys>integer</MaxKeys>\n"
                             "   <CommonPrefixes>\n"
                             "      <Prefix>string</Prefix>\n"
                             "   </CommonPrefixes>\n"
                             "   <EncodingType>string</EncodingType>\n"
                             "   <KeyCount>integer</KeyCount>\n"
                             "   <ContinuationToken>string</ContinuationToken>\n"
                             "   <NextContinuationToken>string</NextContinuationToken>\n"
                             "   <StartAfter>string</StartAfter>\n"
                             "</ListBucketResult>"));

        m_res.prepare_payload();
        return m_res;

    }

} // namespace uh::cluster::rest::http::model
