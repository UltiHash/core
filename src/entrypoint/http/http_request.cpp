#include "http_request.h"

#include "boost/url/url.hpp"
#include "common/telemetry/log.h"
#include "common/utils/strings.h"
#include "entrypoint/http/command_exception.h"
#include "entrypoint/utils.h"
#include <charconv>
#include <regex>

using namespace boost;

namespace uh::cluster {

namespace {

boost::urls::url
load_url(const http::request_parser<http::empty_body>::value_type& req) {
    boost::urls::url url;

    auto target = req.target();
    auto query_index = target.find('?');

    if (query_index != std::string::npos) {
        url.set_encoded_query(target.substr(query_index + 1));
        url.set_encoded_path(target.substr(0, query_index));
    } else {
        url.set_encoded_path(target);
    }

    return url;
}

class raw_decoder : public transport_decoder {
public:
    raw_decoder(asio::ip::tcp::socket& s, beast::flat_buffer&& initial,
                std::size_t length)
        : m_socket(s),
          m_buffer(std::move(initial)),
          m_length(length) {}

    coro<std::size_t> read(std::span<char> dest) override {
        auto rv = 0ull;

        if (m_buffer.size() > 0ull) {
            auto count = asio::buffer_copy(asio::buffer(&dest[0], dest.size()),
                                           m_buffer.data());
            m_buffer.consume(count);
            rv += count;
            m_length -= count;
        }

        auto count = std::min(dest.size(), m_length);
        auto read = co_await asio::async_read(
            m_socket, asio::buffer(&dest[rv], count), asio::use_awaitable);

        rv += read;
        m_length -= read;
        co_return rv;
    }

private:
    asio::ip::tcp::socket& m_socket;
    beast::flat_buffer m_buffer;
    std::size_t m_length;
};

class chunked_transport_decoder : public transport_decoder {
public:
    chunked_transport_decoder(asio::ip::tcp::socket& s,
                              const beast::flat_buffer& initial)
        : m_socket(s),
          m_buffer() {
        m_buffer.reserve(BUFFER_SIZE);
        m_buffer.resize(initial.size());
        asio::buffer_copy(asio::buffer(m_buffer),
                          asio::buffer(initial.data(), initial.size()));
    }

    coro<std::size_t> read(std::span<char> dest) override {
        if (m_end) {
            throw std::runtime_error("trying to read past end of data");
        }

        std::size_t rv = 0ull;

        while (rv < dest.size()) {
            if (m_chunk_size == 0ull) {
                m_chunk_size = co_await read_chunk_header();
            }

            if (m_chunk_size == 0ull) {
                m_end = true;
                break;
            }

            auto count = std::min(m_chunk_size, dest.size() - rv);
            auto read = co_await read_data(dest.subspan(rv, count));

            rv += read;
            m_chunk_size -= read;

            co_await read_nl();
        }

        co_return rv;
    }

private:
    coro<void> read_nl() {
        char nl[2];
        std::size_t offset = 0;

        if (!m_buffer.empty()) {
            auto count = std::min(m_buffer.size(), sizeof(nl) - offset);
            memcpy(nl, &m_buffer[0], count);
            m_buffer.erase(m_buffer.begin(), m_buffer.begin() + count);
            offset += count;
        }

        if (offset < sizeof(nl)) {
            co_await asio::async_read(
                m_socket, asio::buffer(&nl[offset], sizeof(nl) - offset),
                asio::use_awaitable);
        }

        if (nl[0] != '\r' || nl[1] != '\n') {
            throw std::runtime_error("newline required");
        }
    }

    std::size_t find_nl() const {
        bool has_cr = false;

        for (auto index = 0ull; index < m_buffer.size(); ++index) {
            if (m_buffer[index] == '\r') {
                has_cr = true;
                continue;
            }

            if (m_buffer[index] == '\n' && has_cr) {
                return index + 1;
            }

            has_cr = false;
        }

        throw std::runtime_error("newline required");
    }

    coro<std::size_t> read_chunk_header() {
        co_await asio::async_read_until(m_socket,
                                        asio::dynamic_buffer(m_buffer), "\r\n",
                                        asio::use_awaitable);

        auto pos = find_nl();

        std::size_t size = 0ull;
        auto [_, ec] = std::from_chars(&m_buffer[0], &m_buffer[pos], size, 16);

        if (ec != std::errc()) {
            throw std::runtime_error("from_chars failed: " +
                                     make_error_condition(ec).message());
        }

        m_buffer.erase(m_buffer.begin(), m_buffer.begin() + pos);
        co_return size;
    }

    coro<std::size_t> read_data(std::span<char> buffer) {
        std::size_t offs = 0ull;

        if (m_buffer.size() > 0) {
            auto count = std::min(m_buffer.size(), buffer.size());
            memcpy(buffer.data(), m_buffer.data(), count);

            m_buffer.erase(m_buffer.begin(), m_buffer.begin() + count);
            offs += count;
        }

        if (offs < buffer.size()) {
            offs += co_await asio::async_read(
                m_socket,
                asio::buffer(buffer.data() + offs, buffer.size() - offs),
                asio::use_awaitable);
        }

        co_return offs;
    }

    static constexpr std::size_t BUFFER_SIZE = MEBI_BYTE;

    asio::ip::tcp::socket& m_socket;
    std::vector<char> m_buffer;
    std::size_t m_chunk_size = 0ull;
    bool m_end = false;
};

std::unique_ptr<transport_decoder>
make_decoder(const http::request_parser<http::empty_body>::value_type& req,
             asio::ip::tcp::socket& stream, beast::flat_buffer&& initial) {

    /* Amazon will upload data using chunked transfer without explicitly setting
     * the `Transfer-Encoding` header for signed data. This also prevents us
     * from using beasts HTTP parser for decoding the transfer encoding.
     *
     * (see
     * https://docs.aws.amazon.com/AmazonS3/latest/API/sigv4-auth-using-authorization-header.html#sigv4-auth-header-overview)
     */
    auto content_sha = req.base().find("x-amz-content-sha256");
    if (content_sha != req.base().end() &&
        (content_sha->value() == "STREAMING-UNSIGNED-PAYLOAD-TRAILER" ||
         content_sha->value() == "STREAMING-AWS4-HMAC-SHA256-PAYLOAD" ||
         content_sha->value() == "STREAMING-AWS4-HMAC-SHA256-PAYLOAD-TRAILER" ||
         content_sha->value() == "STREAMING-AWS4-ECDSA-P256-SHA256-PAYLOAD" ||
         content_sha->value() ==
             "STREAMING-AWS4-ECDSA-P256-SHA256-PAYLOAD-TRAILER")) {
        return std::make_unique<chunked_transport_decoder>(stream,
                                                           std::move(initial));
    }

    std::size_t length = 0ull;

    auto content_length = req.base().find("content-length");
    if (content_length != req.base().end()) {
        length = std::stoul(content_length->value());
    }

    return std::make_unique<raw_decoder>(stream, std::move(initial), length);
}

} // namespace

coro<std::unique_ptr<http_request>>
http_request::create(context& ctx, asio::ip::tcp::socket& s) {

    http::request_parser<http::empty_body> req;
    boost::beast::flat_buffer buffer;
    req.body_limit((std::numeric_limits<std::uint64_t>::max)());

    co_await beast::http::async_read_header(s, buffer, req,
                                            asio::use_awaitable);

    co_return std::unique_ptr<http_request>(
        new http_request(ctx, s, std::move(req.get()), std::move(buffer)));
}

http_request::http_request(
    context& ctx, boost::asio::ip::tcp::socket& stream,
    http::request_parser<http::empty_body>::value_type&& req,
    beast::flat_buffer&& initial)
    : m_ctx(ctx),
      m_stream(stream),
      m_req(std::move(req)),
      m_decoder(make_decoder(m_req, m_stream, std::move(initial))) {

    if (req.base().version() != 11) {
        throw std::runtime_error(
            "bad http version. support exists only for HTTP 1.1.\n");
    }

    auto url = load_url(m_req);

    for (const auto& param : url.params()) {
        m_params[param.key] = param.value;
    }

    auto keys = extract_bucket_and_object(url);
    m_bucket_id = std::get<0>(keys);
    m_object_key = std::get<1>(keys);
}

http::verb http_request::method() const { return m_req.method(); }

const std::string& http_request::bucket() const { return m_bucket_id; }

const std::string& http_request::object_key() const { return m_object_key; }

coro<std::size_t> http_request::read_body(std::span<char> buffer) {
    return m_decoder->read(buffer);
}

std::optional<std::string> http_request::query(const std::string& name) const {
    if (auto it = m_params.find(name); it != m_params.end()) {
        return it->second;
    }

    return std::nullopt;
}

bool http_request::has_query() const { return !m_params.empty(); }

std::optional<std::string> http_request::header(const std::string& name) const {
    auto it = m_req.base().find(name);
    if (it == m_req.base().end()) {
        return std::nullopt;
    }

    return it->value();
}

std::ostream& operator<<(std::ostream& out, const http_request& req) {
    out << req.m_req.base().method_string() << " " << req.m_req.base().target()
        << " ";

    std::string delim;
    for (const auto& field : req.m_req.base()) {
        out << delim << field.name_string() << ": " << field.value();
        delim = ", ";
    }

    return out;
}

} // namespace uh::cluster
