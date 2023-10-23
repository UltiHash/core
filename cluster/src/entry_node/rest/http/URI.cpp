#include "URI.h"
#include <utility>
#include <iostream>

namespace uh::cluster::rest::http
{

    URI::URI(const http::request_parser<http::empty_body>& req) : m_req(req), m_target_string(req.get().target())
    {
        extract_and_set_bucket_id_and_object_key();
        extract_and_set_query_strings();
    }

    std::string URI::get_bucket_id() const
    {
        return m_bucket_id;
    }

    std::string URI::get_object_key() const
    {
        return m_object_key;
    }

    std::string URI::get_query_string_value(const std::string& key) const
    {
        auto index = m_query_string.find(key+'=');
        if (index ==  std::string::npos)
        {
            return "";
        }
        index += key.length()+1;

        auto value_end_index = m_query_string.find_first_of('&', index);
        if(value_end_index != std::string::npos)
        {
            return m_query_string.substr(index, value_end_index - index);
        }
        else
        {
            return m_query_string.substr(index);
        }
    }

    void URI::extract_and_set_bucket_id_and_object_key()
    {
        auto after_bucket_slash = m_target_string.find_first_of('/', 1);
        m_bucket_id = m_target_string.substr(1, after_bucket_slash - 1);

        auto index = m_target_string.find_first_of('?');
        if ( index != std::string::npos)
        {
            m_object_key = m_target_string.substr( after_bucket_slash + 1 , index - after_bucket_slash - 1);
        }
        else
        {
            m_object_key = m_target_string.substr(after_bucket_slash + 1);
        }
    }

    void URI::extract_and_set_query_strings()
    {
        size_t query_start = m_target_string.find('?');

        if (query_start != std::string::npos)
        {
            m_query_string = m_target_string.substr(query_start + 1);
        }
    }

} // uh::cluster::rest::http
