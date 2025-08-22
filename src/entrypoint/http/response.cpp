#include "response.h"
#include "common/coroutines/promise.h"
#include "common/types/common_types.h"
#include "entrypoint/formats.h"
#include "string_body.h"
#include <boost/property_tree/xml_parser.hpp>
#include <sstream>

namespace uh::cluster::ep::http {

namespace asio = boost::asio;

response::response()
    : response(http::status::ok) {}

response::response(http::status status)
    : m_res(status, 11),
      m_body(std::make_unique<string_body>("")) {}

response::response(beast::http::response<beast::http::empty_body> res,
                   std::unique_ptr<http::body> body)
    :  m_res(std::move(res)),
       m_body(std::move(body))
{
}

void response::set_body(std::unique_ptr<http::body>&& body) noexcept {
    m_body = std::move(body);
}

void response::set_original_size(std::size_t original_size) {
    m_res.set("uh-original-size", std::to_string(original_size));
    m_res.set("uh-original-size-mb",
              std::to_string(static_cast<double>(original_size) / MEBI_BYTE));
}

void response::set_effective_size(std::size_t effective_size) {
    m_res.set("uh-effective-size", std::to_string(effective_size));
    m_res.set("uh-effective-size-mb",
              std::to_string(static_cast<double>(effective_size) / MEBI_BYTE));
}

void response::set(const std::string& header,
                   std::optional<std::string> value) {
    if (value) {
        m_res.set(header, *value);
    } else {
        m_res.erase(header);
    }
}

void response::set(const std::string& header,
                   std::optional<std::size_t> value) {
    if (value) {
        m_res.set(header, std::to_string(*value));
    } else {
        m_res.erase(header);
    }
}

std::optional<std::string> response::header(const std::string& name) const {
    auto it = m_res.find(name);
    if (it == m_res.end()) {
        return {};
    }

    return it->value();
}

response& operator<<(response& res, const boost::property_tree::ptree& pt) {
    std::ostringstream ss;

    res.set("Content-Type", "application/xml");
    boost::property_tree::write_xml(ss, pt);

    /**
     * Note about line-ending: leaving the line ending out leads to errors
     * with Apache HTTP client as for certain status codes it tries to ignore
     * XML body skipping forward to next header, which it misses as there is
     * no line ending.
     * With '\r\n' line ending some component (I suppose the same client)
     * converts '\r' to some escape sequence, leading to XML parse errors.
     */
    res.set_body(std::make_unique<string_body>(ss.str() + "\n"));
    return res;
}

std::ostream& operator<<(std::ostream& out, const response& res) {
    const auto& base = res.base();

    out << base.result_int() << " " << base.reason() << ", ";

    std::string delim;
    for (const auto& field : base) {
        out << delim << field.name_string() << ": " << field.value();
        delim = ", ";
    }

    return out;
}

struct visitor {
    void operator()(auto ec, auto buffers) {
        last_bytes = 0;

        for (const auto& b : buffers) {
            auto s = buffer.size();
            buffer.resize(buffer.size() + b.size());
            memcpy(&buffer[s], b.data(), b.size());
            last_bytes += b.size();
        }
    };

    std::vector<char>& buffer;
    std::size_t& last_bytes;
};

coro<void> write(stream& s, response&& res, const std::string& id) {
    auto& body = res.body();

    res.set("Server", "UltiHash");
    res.set("x-amz-request-id", id);

    res.set("Date", imf_fixdate(std::chrono::system_clock::now()));

    if (!res.header("Content-Length")) {
        res.set("Content-Length", body.length());
    }

    std::vector<char> buffer;
    std::size_t last_bytes;
    beast::http::response_serializer<beast::http::empty_body> sr(res.base());
    while (!sr.is_header_done()) {
        beast::error_code ec;
        sr.next(ec, visitor{buffer, last_bytes});
        sr.consume(last_bytes);
        LOG_DEBUG() << s.peer() << ": serializing header, buffer size: " << buffer.size();
    }

    co_await s.write(buffer);
    auto bs = body.buffer_size();

    std::span<const char> data = co_await body.read(bs);
    while (!data.empty()) {
        co_await s.write(data);
        co_await body.consume();
        data = co_await body.read(bs);
    }

    if (static_cast<unsigned>(res.result()) >= 500) {
        LOG_WARN() << s.peer() << ", response sent: " << res;
    } else {
        LOG_DEBUG() << s.peer() << ", response sent: " << res;
    }
}

} // namespace uh::cluster::ep::http
