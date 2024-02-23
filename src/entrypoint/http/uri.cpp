#include "uri.h"
#include "custom_error_response_exception.h"
#include <regex>

namespace uh::cluster {

uri::uri(const http::request_parser<http::empty_body>& req) {
    if (req.get().base().version() != 11) {
        throw std::runtime_error(
            "bad http version. support exists only for HTTP 1.1.\n");
    }

    m_method = req.get().method();

    auto target = req.get().target();

    auto query_index = target.find('?');

    if (query_index != std::string::npos) {
        // extract query string
        m_url.set_encoded_query(target.substr(query_index + 1));

        // extract path
        m_url.set_encoded_path(target.substr(0, query_index));
    } else {
        // extract path
        m_url.set_encoded_path(target.substr(0));
    }

    extract_bucket_and_object();
    extract_query_parameters();
}

const std::string& uri::get_bucket_id() const { return m_bucket_id; }

const std::string& uri::get_object_key() const { return m_object_key; }

method uri::get_method() const { return m_method; }

bool uri::query_string_exists(const std::string& key) const {
    auto itr = m_query_parameters.find(key);
    if (itr != m_query_parameters.end()) {
        return true;
    } else {
        return false;
    }
}

const std::string& uri::get_query_string_value(const std::string& key) const {
    return m_query_parameters.at(key);
}

const std::map<std::string, std::string>& uri::get_query_parameters() const {
    return m_query_parameters;
}

void uri::extract_query_parameters() {
    for (const auto& param : m_url.params()) {
        m_query_parameters[param.key] = param.value;
    }
}

void uri::extract_bucket_and_object() {
    for (const auto& seg : m_url.segments()) {
        if (m_bucket_id.empty())
            m_bucket_id = seg;
        else
            m_object_key += seg + '/';
    }

    if (!m_object_key.empty())
        m_object_key.pop_back();

    // check bucket id and object key for validity
    if (!m_bucket_id.empty()) {
        if (m_bucket_id.size() < 3 || m_bucket_id.size() > 63) {
            throw custom_error_response_exception(
                http::status::bad_request, http_error::invalid_bucket_name);
        }

        std::regex bucket_pattern(
            R"(^(?!(xn--|sthree-|sthree-configurator-))(?!.*-s3alias$)(?!.*--ol-s3$)(?!^(\d{1,3}\.){3}\d{1,3}$)[a-z0-9](?!.*\.\.)(?!.*[.\s-][.\s-])[a-z0-9.-]*[a-z0-9]$)");
        if (!std::regex_match(m_bucket_id, bucket_pattern)) {
            throw custom_error_response_exception(
                http::status::bad_request, http_error::invalid_bucket_name);
        }
    }
}

} // namespace uh::cluster
