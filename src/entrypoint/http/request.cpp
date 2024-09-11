#include "request.h"

#include "entrypoint/utils.h"

using namespace boost;

namespace uh::cluster::ep::http {

request::request(beast::http::request<beast::http::empty_body> headers,
                 std::unique_ptr<body> body, asio::ip::tcp::endpoint peer)
    : m_req(std::move(headers)),
      m_body(std::move(body)),
      m_peer(peer),
      m_ctx() {
    auto target = parse_request_target(m_req.target());
    m_params = std::move(target.params);
    m_path = std::move(target.path);
    m_bucket_id = std::move(target.bucket);
    m_object_key = std::move(target.object);
}

request::request(partial_parse_result& req, std::unique_ptr<body> body)
    : request(std::move(req.headers), std::move(body), req.peer) {}

http::verb request::method() const { return m_req.method(); }

std::string_view request::target() const { return m_req.target(); }
const std::string& request::path() const { return m_path; }

const std::string& request::bucket() const { return m_bucket_id; }

const std::string& request::object_key() const { return m_object_key; }

coro<std::size_t> request::read_body(std::span<char> buffer) {
    return m_body->read(buffer);
}

std::optional<std::string> request::query(const std::string& name) const {
    if (auto it = m_params.find(name); it != m_params.end()) {
        return it->second;
    }

    return std::nullopt;
}

const std::map<std::string, std::string>& request::query_map() const {
    return m_params;
}

const beast::http::fields& request::header() const { return m_req.base(); }

bool request::has_query() const { return !m_params.empty(); }

std::optional<std::string> request::header(const std::string& name) const {
    if (auto it = m_req.base().find(name); it != m_req.base().end()) {
        return it->value();
    }

    return {};
}

const uh::cluster::context& request::context() const { return m_ctx; }

uh::cluster::context& request::context() { return m_ctx; }

const user::user& request::authenticated_user() const {
    return m_authenticated_user;
}

void request::authenticated_user(user::user user) {
    m_authenticated_user = std::move(user);
}

std::ostream& operator<<(std::ostream& out, const request& req) {
    out << req.m_req.base().method_string() << " " << req.m_req.base().target()
        << " ";

    std::string delim;
    for (const auto& field : req.m_req.base()) {
        out << delim << field.name_string() << ": " << field.value();
        delim = ", ";
    }

    return out;
}

} // namespace uh::cluster::ep::http
