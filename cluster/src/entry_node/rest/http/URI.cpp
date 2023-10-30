#include "URI.h"
#include <regex>
#include "entry_node/rest/utils/string/string_utils.h"

namespace uh::cluster::rest::http
{

    const char* to_string(scheme scheme)
    {
        switch (scheme)
        {
            case scheme::HTTP:
                return "http";
            case scheme::HTTPS:
                return "https";
            default:
                return "http";
        }
    }

    scheme from_string(const char* name)
    {
        std::string lower_case_string  = rest::utils::string_utils::to_lower(name);

        if (lower_case_string == "http")
        {
            return scheme::HTTP;
        }
        else if (lower_case_string == "https")
        {
            return scheme::HTTPS;
        }

        return scheme::HTTPS;
    }

    URI::URI(const http::request_parser<http::empty_body>& req) : m_req(req)
    {
        if (m_req.get().base().version() != 11)
        {
            throw std::runtime_error("bad http version. support exists only for HTTP 1.1.\n");
        }

        m_method = get_http_method_from_beast(req.get().method());

        auto target = m_req.get().target();

        auto query_index = target.find('?');

        if (query_index != std::string::npos)
        {
            // extract query string
            m_url.set_encoded_query(target.substr(query_index + 1));

            // extract path
            m_url.set_encoded_path(target.substr(0, query_index));
        }
        else
        {
            // extract path
            m_url.set_encoded_path(target.substr(0));
        }

        extract_and_set_bucket_id_and_object_key();
        extract_and_set_query_parameters();
    }

    const std::string& URI::get_bucket_id() const
    {
        return m_bucket_id;
    }

    const std::string& URI::get_object_key() const
    {
        return m_object_key;
    }

    [[nodiscard]] http_method URI::get_http_method() const
    {
        return m_method;
    }


    bool URI::query_string_exists(const std::string& key) const
    {
        auto itr = m_query_parameters.find(key);
        if (itr != m_query_parameters.end())
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    const std::string& URI::get_query_string_value(const std::string& key) const
    {
        return m_query_parameters.at(key);
    }

    const std::map<std::string, std::string>& URI::get_query_parameters() const
    {
        return m_query_parameters;
    }

    void URI::extract_and_set_query_parameters()
    {
        for (const auto& param : m_url.params())
        {
            m_query_parameters[param.key] = param.value;
        }
    }

    void URI::extract_and_set_bucket_id_and_object_key()
    {
        for (const auto& seg : m_url.segments())
        {
            if (m_bucket_id.empty())
                m_bucket_id = seg;
            else
                m_object_key = seg + '/';
        }

        if (!m_object_key.empty())
            m_object_key.pop_back();

        // check bucket id and object key for validity
        {
            if (m_bucket_id.size() < 3 || m_bucket_id.size() > 63 )
                throw std::runtime_error("invalid bucket name length");

            std::regex bucket_pattern(R"(^(?!(xn--|sthree-|sthree-configurator-))(?!.*-s3alias$)(?!.*--ol-s3$)(?!^(\d{1,3}\.){3}\d{1,3}$)[a-z0-9](?!.*\.\.)[a-z0-9.-]*[a-z0-9]$)");
            if (!std::regex_match(m_bucket_id, bucket_pattern))
                throw std::runtime_error("invalid bucket name");
        }



    }

} // uh::cluster::rest::http
