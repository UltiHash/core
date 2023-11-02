#include "list_objectsv2_response.h"

namespace uh::cluster::rest::http::model
{

    list_objectsv2_response::list_objectsv2_response(const http_request& req) : http_response(req)
    {
        m_res.set(boost::beast::http::field::content_type, "application/xml");
        populate_response_headers();
    }

    list_objectsv2_response::list_objectsv2_response(const http_request& req, http::response<http::string_body> recv_res) :
            http_response(req, std::move(recv_res))
    {
        m_res.set(boost::beast::http::field::content_type, "application/xml");
        populate_response_headers();
    }

    void list_objectsv2_response::add_content(std::string content)
    {
        m_contents.emplace_back(std::move(content));
        m_contentsHasBeenSet = true;
        m_keyCountHasBeenSet = true;
    }

    void list_objectsv2_response::add_name(std::string bucket_name)
    {
        m_name = std::move(bucket_name);
        m_nameHasBeenSet = true;
    }

    void list_objectsv2_response::populate_response_headers()
    {
        auto URI = m_orig_req.get_URI();
        if (URI.query_string_exists("max-keys"))
        {
            m_maxKeys = std::stoi(URI.get_query_string_value("max-keys"));
            m_maxKeysHasBeenSet = true;
        }

        if (URI.query_string_exists("start-after"))
        {
            m_startAfter = URI.get_query_string_value("start-after");
            m_startAfterHasBeenSet = true;
        }
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

        size_t till_marker_count = 0;
        if (m_contentsHasBeenSet)
        {
            int counter = 0;

            auto content_itr = m_contents.begin();
            if (m_startAfterHasBeenSet)
            {
                content_itr = std::find(m_contents.begin(), m_contents.end(), m_startAfter);

                if (content_itr != m_contents.end())
                {
                    content_itr ++;
                }
            }

            for (auto it = m_contents.begin(); it != content_itr ; it++)
            {
                till_marker_count++;
            }

            for (; content_itr != m_contents.end() ; content_itr++)
            {
                content_xml_string +="<Contents>\n"
                                     "<Key>" + *content_itr + "</Key>\n"
                                     "</Contents>\n";

                counter++;
                if ( m_maxKeysHasBeenSet && m_maxKeys == counter)
                    break;
            }
        }

        std::string key_count_xml;
        if (m_keyCountHasBeenSet)
        {
            content_xml_string += "<KeyCount>" + std::to_string(m_contents.size()) + "</KeyCount>\n";
        }

        std::string name_xml_string;
        if (m_nameHasBeenSet)
        {
            name_xml_string += "<Name>" + m_name + "</Name>\n";
        }

        if (m_maxKeysHasBeenSet)
        {
            if ( m_contents.size() - till_marker_count > m_maxKeys)
                m_isTruncated = "true";
        }

        set_body(std::string("<ListBucketResult>\n"
                             "<IsTruncated>" + m_isTruncated + "</IsTruncated>\n"
                             + content_xml_string
                             + name_xml_string +
                             "</ListBucketResult>"));

        m_res.prepare_payload();
        return m_res;

    }

} // namespace uh::cluster::rest::http::model
