#include "get_object_attributes_response.h"

namespace uh::cluster::rest::http::model
{

    get_object_attributes_response::get_object_attributes_response(const http_request& req) : http_response(req)
    {
        m_res.set(boost::beast::http::field::content_type, "application/xml");
    }

    get_object_attributes_response::get_object_attributes_response(const http_request& req, http::response<http::string_body> recv_res) :
            http_response(req, std::move(recv_res))
    {
        m_res.set(boost::beast::http::field::content_type, "application/xml");
    }

    void get_object_attributes_response::set_etag(std::string etag)
    {
        m_eTagHasBeenSet = true;
        m_eTag = std::move(etag);
    }

    const http::response<http::string_body>& get_object_attributes_response::get_response_specific_object()
    {

        if(m_errorHasBeenSet)
        {
            m_error.prepare_payload();
            return m_error;
        }

        if(m_deleteMarkerHasBeenSet)
        {
            m_res.set("x-amz-delete-marker", std::to_string(m_deleteMarker));
        }

        if (m_lastModifiedHasBeenSet)
        {
            m_res.set(boost::beast::http::field::last_modified, m_lastModified);
        }

        if (m_versionIdHasBeenSet)
        {
            m_res.set("x-amz-version-id", m_versionId);
        }

        if (m_requestChargedHasBeenSet)
        {
            m_res.set("x-amz-request-charged", m_requestCharged);
        }

        set_body(std::string("<GetObjectAttributesOutput>\n"
                             "   <ETag>string</ETag>\n"
                             "   <Checksum>\n"
                             "      <ChecksumCRC32>string</ChecksumCRC32>\n"
                             "      <ChecksumCRC32C>string</ChecksumCRC32C>\n"
                             "      <ChecksumSHA1>string</ChecksumSHA1>\n"
                             "      <ChecksumSHA256>string</ChecksumSHA256>\n"
                             "   </Checksum>\n"
                             "   <ObjectParts>\n"
                             "      <IsTruncated>boolean</IsTruncated>\n"
                             "      <MaxParts>integer</MaxParts>\n"
                             "      <NextPartNumberMarker>integer</NextPartNumberMarker>\n"
                             "      <PartNumberMarker>integer</PartNumberMarker>\n"
                             "      <Part>\n"
                             "         <ChecksumCRC32>string</ChecksumCRC32>\n"
                             "         <ChecksumCRC32C>string</ChecksumCRC32C>\n"
                             "         <ChecksumSHA1>string</ChecksumSHA1>\n"
                             "         <ChecksumSHA256>string</ChecksumSHA256>\n"
                             "         <PartNumber>integer</PartNumber>\n"
                             "         <Size>long</Size>\n"
                             "      </Part>\n"
                             "      <PartsCount>integer</PartsCount>\n"
                             "   </ObjectParts>\n"
                             "   <StorageClass>string</StorageClass>\n"
                             "   <ObjectSize>long</ObjectSize>\n"
                             "</GetObjectAttributesOutput>"));

        m_res.prepare_payload();
        return m_res;

    }

} // namespace uh::cluster::rest::http::model
