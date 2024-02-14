#include "http_response.h"

namespace uh::cluster {

void http_response::set_body(std::string&& body) {
    m_res.body() = std::move(body);

    set_etag(md5::calculateMD5(m_res.body()));
}

void http_response::set_effective_size(std::size_t effective_size) {
    m_res.set("uh-original-size", std::to_string(m_res.body().size()));
    m_res.set("uh-original-size-mb",
              std::to_string(static_cast<double>(m_res.body().size()) /
                             (1024 * 1024)));

    m_res.set("uh-effective-size", std::to_string(effective_size));
    m_res.set(
        "uh-effective-size-mb",
        std::to_string(static_cast<double>(effective_size) / (1024 * 1024)));
}

void http_response::set_space_savings(std::size_t space_savings) {
    m_res.set("uh-space-savings-ratio", std::to_string(space_savings));
}

const http::response<http::string_body>&
http_response::get_prepared_response() {
    m_res.prepare_payload();
    return m_res;
}

void http_response::set_bandwidth(double bandwidth) {
    m_res.set("uh-bandwidth-mbps", std::to_string(bandwidth));
}

void http_response::set_etag(std::string etag) { m_etag = std::move(etag); }

} // namespace uh::cluster