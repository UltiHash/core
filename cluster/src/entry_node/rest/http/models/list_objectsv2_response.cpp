#include "list_objectsv2_response.h"

namespace uh::cluster::rest::http::model
{

    list_objectsv2_response::list_objectsv2_response(const http_request& req) : http_response(req)
    {
        m_res.set(boost::beast::http::field::content_type, "application/xml");
    }

    list_objectsv2_response::list_objectsv2_response(const http_request& req, http::response<http::string_body> recv_res) :
            http_response(req, std::move(recv_res))
    {
        m_res.set(boost::beast::http::field::content_type, "application/xml");
    }

    void list_objectsv2_response::add_content(std::string content)
    {
        m_contents.emplace_back(std::move(content));
        m_contentsHasBeenSet = true;
    }

    void list_objectsv2_response::add_name(std::string bucket_name)
    {
        m_name = std::move(bucket_name);
        m_nameHasBeenSet = true;
    }

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

        std::string content_xml_string;
        if (m_contentsHasBeenSet)
        {
            for (const auto& bucket: m_contents)
            {
                content_xml_string +="<Contents>\n"
                                     "<Key>" + bucket + "</Key>\n"
                                     "</Contents>\n";
            }
        }

        std::string name_xml_string;
        if (m_nameHasBeenSet)
        {
            name_xml_string += "<Name>" + m_name + "<Name>\n";
        }

        set_body(std::string("<ListBucketResult>\n"
                             + content_xml_string
                             + name_xml_string +
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
