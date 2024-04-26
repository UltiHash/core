#include "uri.h"
#include "command_exception.h"
#include <regex>

namespace uh::cluster {

uri::uri(const http::request_parser<http::empty_body>::value_type& req) {
    if (req.base().version() != 11) {
        throw std::runtime_error(
            "bad http version. support exists only for HTTP 1.1.\n");
    }

    auto target = req.target();
    auto query_index = target.find('?');

    if (query_index != std::string::npos) {
        m_url.set_encoded_query(target.substr(query_index + 1));
        m_url.set_encoded_path(target.substr(0, query_index));
    } else {
        m_url.set_encoded_path(target);
    }

    extract_bucket_and_object();
    extract_query_parameters();
}

const std::string& uri::bucket() const { return m_bucket_id; }

const std::string& uri::object_key() const { return m_object_key; }

const std::string& uri::get(const std::string& name) const {
    return m_params.at(name);
}

std::optional<std::string> uri::get_opt(const std::string& name) const {
    if (auto it = m_params.find(name); it != m_params.end()) {
        return it->second;
    }

    return std::nullopt;
}

bool uri::empty() const { return m_params.empty(); }

bool uri::has(const std::string& name) const { return m_params.contains(name); }

void uri::extract_query_parameters() {
    for (const auto& param : m_url.params()) {
        m_params[param.key] = param.value;
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

    if (!m_bucket_id.empty()) {
        if (m_bucket_id.size() < 3 || m_bucket_id.size() > 63) {
            throw command_exception(http::status::bad_request,
                                    "InvalidBucketName",
                                    "bucket name has invalid length");
        }

        std::regex bucket_pattern(
            R"(^(?!(xn--|sthree-|sthree-configurator-))(?!.*-s3alias$)(?!.*--ol-s3$)(?!^(\d{1,3}\.){3}\d{1,3}$)[a-z0-9](?!.*\.\.)(?!.*[.\s-][.\s-])[a-z0-9.-]*[a-z0-9]$)");
        if (!std::regex_match(m_bucket_id, bucket_pattern)) {
            throw command_exception(http::status::bad_request,
                                    "InvalidBucketName",
                                    "bucket name has invalid characters");
        }
    }
}

} // namespace uh::cluster
