#include "http_response.h"
#include "common/types/common_types.h"
#include "common/utils/random.h"
#include <boost/property_tree/xml_parser.hpp>
#include <sstream>

namespace uh::cluster {

http_response::http_response()
    : m_res(http::status::ok, 11) {}

http_response::http_response(http::status status)
    : m_res(status, 11) {}

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
    m_res.set("x-amz-request-id", generate_unique_id());
    m_res.prepare_payload();
    return m_res;
}

void http_response::set_etag(const std::string& etag) {
    m_res.set(http::field::etag, etag);
}

http_response& operator<<(http_response& res,
                          const boost::property_tree::ptree& pt) {
    std::ostringstream ss;

    res.base().set("Content-Type", "application/xml");
    boost::property_tree::write_xml(ss, pt);

    /**
     * Note about line-ending: leaving the line ending out leads to errors
     * with Apache HTTP client as for certain status codes it tries to ignore
     * XML body skipping forward to next header, which it misses as there is
     * no line ending.
     * With '\r\n' line ending some component (I suppose the same client)
     * converts '\r' to some escape sequence, leading to XML parse errors.
     */
    res.base().body() += ss.str() + "\n";
    return res;
}

std::ostream& operator<<(std::ostream& out, const http_response& res) {
    const auto& base = res.base();

    out << base.result_int() << " " << base.reason() << ", ";

    std::string delim;
    for (const auto& field : base) {
        out << delim << field.name_string() << ": " << field.value();
        delim = ", ";
    }

    return out;
}

} // namespace uh::cluster
