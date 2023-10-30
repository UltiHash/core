#include "list_objects_response.h"
#include <iostream>
namespace uh::cluster::rest::http::model
{

    list_objects_response::list_objects_response(const http_request& req) : http_response(req)
    {
        m_res.set(boost::beast::http::field::content_type, "application/xml");
        populate_response_headers();
    }

    list_objects_response::list_objects_response(const http_request& req, http::response<http::string_body> recv_res) :
            http_response(req, std::move(recv_res))
    {
        m_res.set(boost::beast::http::field::content_type, "application/xml");
        populate_response_headers();
    }

    void list_objects_response::add_content(std::string content)
    {
        m_contents.emplace_back(std::move(content));
        m_contentsHasBeenSet = true;
    }

    void list_objects_response::add_name(std::string bucket_name)
    {
        m_name = std::move(bucket_name);
        m_nameHasBeenSet = true;
    }

    void list_objects_response::populate_response_headers()
    {
        auto max_keys = m_orig_req.get_URI().get_query_string_value("max-keys");
        if (!max_keys.empty())
        {
            m_maxKeys = std::stoi(max_keys);
            m_maxKeysHasBeenSet = true;
        }

        auto marker = m_orig_req.get_URI().get_query_string_value("marker");
        if (!marker.empty())
        {
            m_marker = marker;
            m_markerHasBeenSet = true;
        }
    }

    const http::response<http::string_body>& list_objects_response::get_response_specific_object()
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
            if (m_markerHasBeenSet)
            {
                content_itr = std::find(m_contents.begin(), m_contents.end(), m_marker);

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
