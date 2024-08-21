#include "http_request.h"

#include "chunked_body.h"
#include "raw_body.h"

#include "boost/url/url.hpp"
#include "common/telemetry/log.h"
#include "common/utils/strings.h"
#include "entrypoint/http/command_exception.h"
#include "entrypoint/utils.h"
#include <regex>

using namespace boost;
using namespace uh::cluster::ep::http;

namespace uh::cluster {

namespace {

std::unique_ptr<ep::http::body>
make_body(const http::request_parser<http::empty_body>::value_type& req,
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
        return std::make_unique<chunked_body>(stream, std::move(initial));
    }

    std::size_t length = 0ull;

    auto content_length = req.base().find("content-length");
    if (content_length != req.base().end()) {
        length = std::stoul(content_length->value());
    }

    return std::make_unique<raw_body>(stream, std::move(initial), length);
}

} // namespace

coro<std::unique_ptr<http_request>>
http_request::create(asio::ip::tcp::socket& s) {

    http::request_parser<http::empty_body> req;
    boost::beast::flat_buffer buffer;
    req.body_limit((std::numeric_limits<std::uint64_t>::max)());

    co_await beast::http::async_read_header(s, buffer, req,
                                            asio::use_awaitable);

    co_return std::unique_ptr<http_request>(
        new http_request(s, std::move(req.get()), std::move(buffer)));
}

http_request::http_request(
    boost::asio::ip::tcp::socket& sock,
    http::request_parser<http::empty_body>::value_type&& req,
    beast::flat_buffer&& initial)
    : m_req(std::move(req)),
      m_body(make_body(m_req, sock, std::move(initial))),
      m_peer(sock.remote_endpoint()),
      m_ctx() {

    if (req.base().version() != 11) {
        throw std::runtime_error(
            "bad http version. support exists only for HTTP 1.1.\n");
    }

    auto target = m_req.target();
    auto query_index = target.find('?');

    boost::urls::url url;
    if (query_index != std::string::npos) {
        m_path = target.substr(0, query_index);
        url.set_encoded_path(m_path);
        m_query = target.substr(query_index + 1);
        url.set_encoded_query(m_query);
    } else {
        m_path = target;
        url.set_encoded_path(m_path);
    }

    for (const auto& param : url.params()) {
        m_params[param.key] = param.value;
    }

    auto keys = extract_bucket_and_object(url);
    m_bucket_id = std::get<0>(keys);
    m_object_key = std::get<1>(keys);
}

http::verb http_request::method() const { return m_req.method(); }

std::string_view http_request::target() const { return m_req.target(); }
const std::string& http_request::query() const { return m_query; }
const std::string& http_request::path() const { return m_path; }

const std::string& http_request::bucket() const { return m_bucket_id; }

const std::string& http_request::object_key() const { return m_object_key; }

coro<std::size_t> http_request::read_body(std::span<char> buffer) {
    return m_body->read(buffer);
}

std::optional<std::string> http_request::query(const std::string& name) const {
    if (auto it = m_params.find(name); it != m_params.end()) {
        return it->second;
    }

    return std::nullopt;
}

const std::map<std::string, std::string>& http_request::query_map() const {
    return m_params;
}

const beast::http::fields& http_request::header() const { return m_req.base(); }

bool http_request::has_query() const { return !m_params.empty(); }

std::optional<std::string> http_request::header(const std::string& name) const {
    auto it = m_req.base().find(name);
    if (it == m_req.base().end()) {
        return std::nullopt;
    }

    return it->value();
}

const uh::cluster::context& http_request::context() const { return m_ctx; }

uh::cluster::context& http_request::context() { return m_ctx; }

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
