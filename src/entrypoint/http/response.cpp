#include "response.h"
#include "common/coroutines/promise.h"
#include "common/types/common_types.h"
#include "common/utils/double_buffer.h"
#include "entrypoint/formats.h"
#include "string_body.h"
#include <boost/property_tree/xml_parser.hpp>
#include <sstream>

namespace uh::cluster::ep::http {

static constexpr std::size_t BUFFER_CHUNK_SIZE = 32 * MEBI_BYTE;

namespace asio = boost::asio;

response::response()
    : response(http::status::ok) {}

response::response(http::status status)
    : m_res(status, 11),
      m_body(std::make_unique<string_body>("")) {}

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

coro<void> write(asio::ip::tcp::socket& out, response&& res,
                 const std::string& id) {
    auto& body = res.body();

    res.set("Server", "UltiHash");
    res.set("x-amz-request-id", id);

    res.set("Date", imf_fixdate(std::chrono::system_clock::now()));

    if (!res.header("Content-Length")) {
        res.set("Content-Length", body.length());
    }

    beast::http::response_serializer<beast::http::empty_body> sr(res.base());
    beast::http::write_header(out, sr);

    std::optional<future<std::size_t>> fut;

    std::size_t buffer_size =
        body.length() && *body.length() < BUFFER_CHUNK_SIZE ? *body.length()
                                                            : BUFFER_CHUNK_SIZE;
    double_buffer buffers(buffer_size);

    while (body.length() > 0) {
        auto& buffer = buffers.current();
        buffer.resize(buffer.capacity());

        auto count = co_await res.body().read(buffer);
        buffer.resize(count);

        if (fut) {
            co_await fut->get();
        }

        promise<std::size_t> p;
        fut = p.get_future();
        asio::async_write(out, asio::buffer(buffer), use_promise(std::move(p)));

        buffers.flip();
    }

    if (fut) {
        co_await fut->get();
        fut.reset();
    }

    if (static_cast<unsigned>(res.result()) >= 500) {
        LOG_WARN() << out.remote_endpoint() << ", response sent: " << res;
    } else {
        LOG_DEBUG() << out.remote_endpoint() << ", response sent: " << res;
    }
}

} // namespace uh::cluster::ep::http
