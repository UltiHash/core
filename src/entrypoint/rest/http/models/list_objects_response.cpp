#include <iostream>
#include <set>
#include "entrypoint/rest/utils/string/string_utils.h"
#include "list_objects_response.h"

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
        auto URI = m_orig_req.get_URI();

        if (URI.query_string_exists("delimiter"))
        {
            m_delimiter = URI.get_query_string_value("delimiter");
            if (!m_delimiter.empty())
                m_delimiterHasBeenSet = true;
        }

        if (URI.query_string_exists("encoding-type"))
        {
            m_encodingType = URI.get_query_string_value("encoding-type");
            if (!m_encodingType.empty())
                m_encodingTypeHasBeenSet = true;
        }

        if (URI.query_string_exists("marker"))
        {
            m_marker = URI.get_query_string_value("marker");
            m_markerHasBeenSet = true;
        }

        if (URI.query_string_exists("max-keys"))
        {
            m_maxKeys = std::stoi(URI.get_query_string_value("max-keys"));
            m_maxKeysHasBeenSet = true;
        }

        if (URI.query_string_exists("prefix"))
        {
            m_prefix = URI.get_query_string_value("prefix");
            m_prefixHasBeenSet = true;
        }
    }

    const http::response<http::string_body>& list_objects_response::get_response_specific_object()
    {
        if(m_requestChargedHasBeenSet)
        {
            m_res.set("x-amz-request-charged", m_requestCharged);
        }

        if (m_prefixHasBeenSet && !m_prefix.empty())
        {
            std::vector<std::string> filtered_by_prefix;
            std::copy_if(m_contents.begin(), m_contents.end(), std::back_inserter(filtered_by_prefix),
                         [&](const std::string& s) {
                             return s.substr(0, m_prefix.length()) == m_prefix;
                         });
            m_contents = std::move(filtered_by_prefix);
        }

        std::string content_xml_string;
        std::set<std::string> common_prefixes {};

        size_t till_marker_count = 0;
        int counter = 0;

        if (m_contentsHasBeenSet && m_maxKeys != 0)
        {
            auto content_itr = m_contents.begin();
            if (m_markerHasBeenSet)
            {
                auto index_itr = utils::string_utils::find_lexically_closest(m_contents, m_marker);

                if (index_itr != m_contents.end())
                {
                    content_itr = index_itr++;
                }
            }

            till_marker_count = content_itr - m_contents.begin();

            for (; content_itr != m_contents.end(); content_itr++)
            {
                size_t delimiter_found_index;
                if (m_prefixHasBeenSet)
                {
                    delimiter_found_index = content_itr->find(m_delimiter, m_prefix.size());
                }
                else
                {
                    delimiter_found_index = content_itr->find(m_delimiter);
                }

                if (m_delimiterHasBeenSet && delimiter_found_index != std::string::npos)
                {
                    auto prefix = content_itr->substr(0, delimiter_found_index+1);
                    common_prefixes.emplace((m_encodingTypeHasBeenSet ? rest::utils::string_utils::URL_encode(prefix) : prefix));
                }
                else
                {
                    content_xml_string += "<Contents>\n"
                                          "<Key>" + (m_encodingTypeHasBeenSet ? rest::utils::string_utils::URL_encode(*content_itr) : *content_itr) + "</Key>\n" +
                                          "</Contents>\n";

                    counter++;
                }
                if ( counter + common_prefixes.size() == m_maxKeys )
                    break;
            }
        }

        // common prefixes string
        std::string common_prefixes_xml_string;
        for (const auto &prefix: common_prefixes)
        {
            common_prefixes_xml_string += "<CommonPrefixes>\n<Prefix>" + prefix + "</Prefix>\n</CommonPrefixes>\n";
        }

        std::string delimiter_xml_string;
        if (m_delimiterHasBeenSet)
        {
            delimiter_xml_string = "<Delimiter>" + (m_encodingTypeHasBeenSet ? rest::utils::string_utils::URL_encode(m_delimiter) : m_delimiter ) + "</Delimiter>\n";
        }

        std::string key_count_xml;
        if (m_keyCountHasBeenSet)
        {
            content_xml_string += "<KeyCount>" + std::to_string(counter+common_prefixes.size()) + "</KeyCount>\n";
        }

        std::string name_xml_string;
        if (m_nameHasBeenSet)
        {
            name_xml_string += "<Name>" + m_name + "</Name>\n";
        }

        std::string next_marker_xml_string;
        if ( (m_contents.size() - till_marker_count > m_maxKeys) && m_maxKeys != 0 )
        {
            m_isTruncated = "true";
            next_marker_xml_string = "<NextMarker>" + m_contents[till_marker_count + m_maxKeys] + "</NextMarker>";
        }

        std::string max_keys_xml_string = "<MaxKeys>" + std::to_string(m_maxKeys) + "</MaxKeys>\n";

        std::string encoding_type_xml_string;
        if (m_encodingTypeHasBeenSet)
        {
            encoding_type_xml_string = "<EncodingType>" + m_encodingType + "</EncodingType>\n";
        }

        std::string prefix_xml_string;
        if (m_prefixHasBeenSet)
        {
            prefix_xml_string = "<Prefix>" + (m_encodingTypeHasBeenSet ? rest::utils::string_utils::URL_encode(m_prefix) : m_prefix ) + "</Prefix>\n";
        }

        std::string marker_xml_string;
        if (m_markerHasBeenSet)
        {
//            (m_encodingTypeHasBeenSet ? rest::utils::string_utils::URL_encode(m_startAfter) : m_startAfter )
            marker_xml_string = "<StartAfter>" + m_marker + "</StartAfter>\n";
        }

        set_body(std::string("<ListBucketResult>\n"
                             "<IsTruncated>" + m_isTruncated + "</IsTruncated>\n" +
                             marker_xml_string +
                             next_marker_xml_string +
                             content_xml_string +
                             name_xml_string +
                             prefix_xml_string +
                             delimiter_xml_string +
                             max_keys_xml_string +
                             common_prefixes_xml_string +
                             encoding_type_xml_string +
                             key_count_xml +
                             "</ListBucketResult>"));

        m_res.prepare_payload();
        return m_res;

    }

} // namespace uh::cluster::rest::http::model
