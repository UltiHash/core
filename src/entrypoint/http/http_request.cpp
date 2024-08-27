#include "http_request.h"

#include "boost/url/url.hpp"
#include "entrypoint/utils.h"

using namespace boost;
using namespace uh::cluster::ep::http;

namespace uh::cluster {

coro<read_request_result> read_beast_request(asio::ip::tcp::socket& sock) {

    beast::http::request_parser<beast::http::empty_body> parser;
    beast::flat_buffer buffer;
    parser.body_limit((std::numeric_limits<std::uint64_t>::max)());

    co_await beast::http::async_read_header(sock, buffer, parser,
                                            asio::use_awaitable);

    auto req = std::move(parser.get());
    if (req.base().version() != 11) {
        throw std::runtime_error(
            "bad http version. support exists only for HTTP 1.1.\n");
    }

    co_return read_request_result{req, buffer};
}

url_parsing_result parse_request_target(const std::string& target) {
    auto query_index = target.find('?');

    boost::urls::url url;
    if (query_index != std::string::npos) {
        url.set_encoded_path(target.substr(0, query_index));
        url.set_encoded_query(target.substr(query_index + 1));
    } else {
        url.set_encoded_path(target);
    }

    auto keys = extract_bucket_and_object(url);

    url_parsing_result rv;
    rv.path = url.path();
    rv.encoded_path = url.encoded_path();

    for (const auto& param : url.params()) {
        rv.params[param.key] = param.value;
    }

    rv.bucket = std::move(std::get<0>(keys));
    rv.object = std::move(std::get<1>(keys));
    return rv;
}

http_request::http_request(beast::http::request<http::empty_body>&& req,
                           std::unique_ptr<ep::http::body> body,
                           boost::asio::ip::tcp::endpoint peer)
    : m_req(std::move(req)),
      m_body(std::move(body)),
      m_peer(peer),
      m_ctx() {

    auto target = parse_request_target(m_req.target());
    m_params = std::move(target.params);
    m_path = std::move(target.path);
    m_bucket_id = std::move(target.bucket);
    m_object_key = std::move(target.object);
}

http::verb http_request::method() const { return m_req.method(); }

std::string_view http_request::target() const { return m_req.target(); }
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
