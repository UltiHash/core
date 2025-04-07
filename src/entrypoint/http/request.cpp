#include "request.h"

#include "entrypoint/utils.h"

using namespace boost;

namespace uh::cluster::ep::http {

request::request(raw_request req, std::unique_ptr<body> body,
                 ep::user::user user)
    : m_req(std::move(req.headers)),
      m_body(std::move(body)),
      m_authenticated_user(std::move(user)),
      m_peer(req.peer),
      m_bucket_id(get_bucket_id(req.path)),
      m_object_key(get_object_key(req.path)),
      m_params(std::move(req.params)),
      m_path(std::move(req.path)) {}

http::verb request::method() const { return m_req.method(); }

std::string_view request::target() const { return m_req.target(); }
const std::string& request::path() const { return m_path; }

const std::string& request::bucket() const { return m_bucket_id; }

const std::string& request::object_key() const { return m_object_key; }

std::string request::arn() const {
    if (m_object_key.size() != 0)
        return "arn:aws:s3:::" + m_bucket_id + "/" + m_object_key;
    else
        return "arn:aws:s3:::" + m_bucket_id;
}

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

void request::set_query_params(const std::string& query) {
    boost::urls::url url;
    url.set_encoded_query(query);

    std::map<std::string, std::string> params;
    for (const auto& param : url.params()) {
        params[param.key] = param.value;
    }

    m_params = std::move(params);
}

const beast::http::fields& request::header() const { return m_req.base(); }

bool request::has_query() const { return !m_params.empty(); }

std::optional<std::string> request::header(const std::string& name) const {
    if (auto it = m_req.base().find(name); it != m_req.base().end()) {
        return it->value();
    }

    return {};
}

const user::user& request::authenticated_user() const {
    return m_authenticated_user;
}

std::string get_bucket_id(const std::string& path) {
    auto segments = split(path, '/');
    return std::string(segments.size() >= 2 ? segments[1] : "");
}

std::string get_object_key(const std::string& path) {
    auto segments = split(path, '/');
    std::string key = segments.size() >= 3
                          ? join(std::views::counted(segments.begin() + 2,
                                                     segments.size() - 2),
                                 "/")
                          : "";
    return std::string(key);
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
