#include "delete_objects_response.h"

namespace uh::cluster::rest::http::model
{

    delete_objects_response::delete_objects_response(const http_request& req) : http_response(req)
    {
        m_res.set(boost::beast::http::field::content_type, "application/xml");
    }

    delete_objects_response::delete_objects_response(const http_request& req, http::response<http::string_body> recv_res) :
            http_response(req, std::move(recv_res))
    {
        m_res.set(boost::beast::http::field::content_type, "application/xml");
    }

    const http::response<http::string_body>& delete_objects_response::get_response_specific_object()
    {

        if(m_errorHasBeenSet)
        {
            m_error.prepare_payload();
            return m_error;
        }

        if (m_requestChargedHasBeenSet)
        {
            m_res.set("x-amz-request-charged", m_requestCharged);
        }

        set_body(std::string("<DeleteResult>\n"
                             "   <Deleted>\n"
                             "      <DeleteMarker>boolean</DeleteMarker>\n"
                             "      <DeleteMarkerVersionId>string</DeleteMarkerVersionId>\n"
                             "      <Key>string</Key>\n"
                             "      <VersionId>string</VersionId>\n"
                             "   </Deleted>\n"
                             "   <Error>\n"
                             "      <Code>string</Code>\n"
                             "      <Key>string</Key>\n"
                             "      <Message>string</Message>\n"
                             "      <VersionId>string</VersionId>\n"
                             "   </Error>\n"
                             "</DeleteResult>"));

        m_res.prepare_payload();
        return m_res;

    }

} // namespace uh::cluster::rest::http::model
