#include "URI.h"
#include <utility>
#include <iostream>

namespace uh::cluster::rest::http
{

    URI::URI(const http::request_parser<http::empty_body>& req) : m_req(req), m_target_string(req.get().target())
    {
        extract_and_set_bucket_id_and_object_key();
        extract_and_set_query_parameters();
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

    void URI::extract_and_set_query_parameters()
    {
        extract_and_set_query_string();

        if (!m_query_string.empty())
        {
            size_t currentPos = 0, locationOfNextDelimiter = 1;

            while (currentPos < m_query_string.size())
            {
                locationOfNextDelimiter = m_query_string.find('&', currentPos);

                std::string keyValuePair;

                if (locationOfNextDelimiter != std::string::npos)
                {
                    keyValuePair = m_query_string.substr(currentPos, locationOfNextDelimiter - currentPos);
                }
                else
                {
                    keyValuePair = m_query_string.substr(currentPos);
                }

                size_t locationOfEquals = keyValuePair.find('=');
                std::string key = keyValuePair.substr(0, locationOfEquals);
                std::string value = keyValuePair.substr(locationOfEquals + 1);

//                if(decode)
//                {
//                    m_query_parameters[string_utils::URLDecode(key.c_str())] = string_utils::URLDecode(value.c_str()));
//                }
//                else
//                {
                    m_query_parameters[key] = value;
//                }

                currentPos += keyValuePair.size() + 1;
            }
        }
    }

    void URI::extract_and_set_bucket_id_and_object_key()
    {
        auto index = m_target_string.find_first_of('?');

        auto after_bucket_slash = m_target_string.find_first_of('/', 1);
        if (after_bucket_slash == std::string::npos)
        {
            if ( index != std::string::npos)
            {
                m_bucket_id = m_target_string.substr( 1 , index - 1);
            }
            else
            {
                m_bucket_id = m_target_string.substr(1);
            }
        }
        else
        {
            m_bucket_id = m_target_string.substr(1, after_bucket_slash - 1);

            if ( index != std::string::npos)
            {
                m_object_key = m_target_string.substr( after_bucket_slash + 1 , index - after_bucket_slash - 1);
            }
            else
            {
                m_object_key = m_target_string.substr(after_bucket_slash + 1);
            }
        }
    }

    void URI::extract_and_set_query_string()
    {
        size_t query_start = m_target_string.find('?');

        if (query_start != std::string::npos)
        {
            m_query_string = m_target_string.substr(query_start + 1);
        }
    }

} // uh::cluster::rest::http
