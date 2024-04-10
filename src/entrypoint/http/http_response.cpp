#include "http_response.h"
#include "common/types/common_types.h"

namespace uh::cluster {

void http_response::set_body(std::string&& body) noexcept {
    m_res.body() = std::move(body);
}

void http_response::set_original_size(std::size_t original_size) {
    m_res.set("uh-original-size", std::to_string(original_size));
    m_res.set("uh-original-size-mb",
              std::to_string(static_cast<double>(original_size) / MEBI_BYTE));
}

void http_response::set_effective_size(std::size_t effective_size) {
    m_res.set("uh-effective-size", std::to_string(effective_size));
    m_res.set("uh-effective-size-mb",
              std::to_string(static_cast<double>(effective_size) / MEBI_BYTE));
}

const http::response<http::string_body>&
http_response::get_prepared_response() {
    m_res.set(http::field::server, "UltiHash");
    m_res.prepare_payload();
    return m_res;
}

void http_response::set_etag(const std::string& etag) {
    m_res.set(http::field::etag, etag);
}

} // namespace uh::cluster
