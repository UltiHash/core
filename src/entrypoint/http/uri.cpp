#include "uri.h"
#include "command_exception.h"

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

    extract_query_parameters();
}

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

} // namespace uh::cluster
