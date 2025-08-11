#include "request.h"

#include "entrypoint/utils.h"

using namespace boost;

namespace uh::cluster::ep::http {

request::request(raw_request rawreq, std::unique_ptr<body> body,
                 ep::user::user user)
    : m_rawreq(std::move(rawreq)),
      m_body(std::move(body)),
      m_authenticated_user(std::move(user)),
      m_bucket_id(get_bucket_id(m_rawreq.path)),
      m_object_key(get_object_key(m_rawreq.path)) {}

verb request::method() const { return m_rawreq.headers.method(); }

std::string_view request::target() const { return m_rawreq.headers.target(); }

const std::string& request::path() const { return m_rawreq.path; }

const std::string& request::bucket() const { return m_bucket_id; }

const std::string& request::object_key() const { return m_object_key; }

std::string request::arn() const {
    if (m_object_key.size() != 0)
        return "arn:aws:s3:::" + m_bucket_id + "/" + m_object_key;
    else
        return "arn:aws:s3:::" + m_bucket_id;
}

const raw_request& request::get_raw_request() const noexcept {
    return m_rawreq;
}

coro<std::size_t> request::read_body(std::span<char> buffer) {
    return m_body->read(buffer);
}

std::vector<boost::asio::const_buffer>
request::get_raw_buffer() const {
    return m_body->get_raw_buffer();
}

boost::asio::ip::tcp::endpoint request::peer() const { return m_rawreq.peer; }

/** Payload that was read while reading the request headers.
 */
std::size_t request::content_length() const {
    return std::stoul(m_rawreq.headers.at("Content-Length"));
}

/**
 * Return value of query parameter specified by `name`. Return
 * `std::nullopt` if parameter is not set.
 */
std::optional<std::string> request::query(const std::string& name) const {
    if (auto it = m_rawreq.params.find(name); it != m_rawreq.params.end()) {
        return it->second;
    }
    return std::nullopt;
}

const std::map<std::string, std::string>& request::query_map() const {
    return m_rawreq.params;
}

void request::set_query_params(const std::string& query) {
    boost::urls::url url;
    url.set_encoded_query(query);

    std::map<std::string, std::string> params;
    for (const auto& param : url.params()) {
        params[param.key] = param.value;
    }

    m_rawreq.params = std::move(params);
}

bool request::has_query() const { return !m_rawreq.params.empty(); }

std::optional<std::string> request::header(const std::string& name) const {
    if (auto it = m_rawreq.headers.find(name); it != m_rawreq.headers.end()) {
        return it->value();
    }
    return {};
}

bool request::keep_alive() const { return m_rawreq.headers.keep_alive(); }

const user::user& request::authenticated_user() const {
    return m_authenticated_user;
}

const beast::http::request<beast::http::empty_body>& request::base() const { return m_rawreq.headers; }

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
    auto rb = req.get_raw_request().get_raw_buffer();
    out << "\n" << std::string{rb.data(), rb.size()} << "\n";

    return out;
}

} // namespace uh::cluster::ep::http
