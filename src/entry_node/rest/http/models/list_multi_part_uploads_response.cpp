#include "list_multi_part_uploads_response.h"
#include "iostream"

namespace uh::cluster::rest::http::model
{

    list_multi_part_uploads_response::list_multi_part_uploads_response(const http_request& req) : http_response(req)
    {
        m_res.set(boost::beast::http::field::content_type, "application/xml");
        populate_response_headers();
    }

    list_multi_part_uploads_response::list_multi_part_uploads_response(const http_request& req, http::response<http::string_body> recv_res) :
            http_response(req, std::move(recv_res))
    {
        m_res.set(boost::beast::http::field::content_type, "application/xml");
        populate_response_headers();
    }

    void list_multi_part_uploads_response::add_key_and_uploadid(const std::string& obj_name, const std::string& upload_id)
    {
        m_keys_and_uploadids.emplace_back(obj_name, upload_id);
    }

    void list_multi_part_uploads_response::populate_response_headers()
    {
        auto URI = m_orig_req.get_URI();

        if (URI.query_string_exists("prefix"))
        {
            m_prefix = URI.get_query_string_value("prefix");
            if (!m_prefix.empty())
                m_prefixHasBeenSet = true;
        }

        if (URI.query_string_exists("delimiter"))
        {
            m_delimiter = URI.get_query_string_value("delimiter");
            if (!m_delimiter.empty())
                m_delimiterHasBeenSet = true;
        }

        if (URI.query_string_exists("max-uploads"))
        {
            m_maxUploads = std::stoi(URI.get_query_string_value("max-uploads"));
            if (m_maxUploads < 0)
                m_maxUploadsHasBeenSet = true;
        }

        if (URI.query_string_exists("key-marker"))
        {
            m_keyMarker = URI.get_query_string_value("key-marker");
            if (m_keyMarker.empty())
                m_keyMarkerHasBeenSet = true;
        }

        if (URI.query_string_exists("upload-id-marker"))
        {
            m_uploadIdMarker = URI.get_query_string_value("upload-id-marker");
            if (!m_uploadIdMarker.empty())
                m_uploadIdMarkerHasBeenSet = true;
        }

        if (URI.query_string_exists("encoding-type"))
        {
            m_encodingType = URI.get_query_string_value("encoding-type");
            if (!m_encodingType.empty())
                m_encodingTypeHasBeenSet = true;
        }
    }

    const http::response<http::string_body>& list_multi_part_uploads_response::get_response_specific_object()
    {

        if(m_errorHasBeenSet)
        {
            m_error.prepare_payload();
            return m_error;
        }

        if(m_expectedBucketOwnerHasBeenSet)
        {
            m_res.set("x-amz-expected-bucket-owner", m_expectedBucketOwner);
        }

        if(m_requestChargedHasBeenSet)
        {
            m_res.set("x-amz-request-payer", m_requestCharged);
        }

        std::string upload_xml_string;
        for (const auto& val : m_keys_and_uploadids)
            upload_xml_string += "<Upload>\n"
                                 "<Key>" + val.object_name + "</Key>\n"
                                 "<UploadId>" + val.upload_id + "</UploadId>\n"
                                 "</Upload>\n";

        set_body(std::string("<ListMultipartUploadsResult>\n"
                             "   <Bucket>" + m_orig_req.get_URI().get_bucket_id() + "</Bucket>\n"
                             + upload_xml_string +
                             "</ListMultipartUploadsResult>"));

        m_res.prepare_payload();
        return m_res;

    }

} // namespace uh::cluster::rest::http::model
