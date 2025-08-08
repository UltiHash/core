#pragma once

#include <common/types/common_types.h>
#include <common/utils/strings.h>
#include <entrypoint/http/body.h>
#include <entrypoint/http/command_exception.h>
#include <entrypoint/http/raw_request.h>
#include <entrypoint/user/user.h>

#include <map>
#include <span>

namespace uh::cluster::ep::http {

inline std::string get_bucket_id(const std::string& path) {
    auto segments = split(path, '/');
    return std::string(segments.size() >= 2 ? segments[1] : "");
}

inline std::string get_object_key(const std::string& path) {
    auto segments = split(path, '/');
    std::string key = segments.size() >= 3
                          ? join(std::views::counted(segments.begin() + 2,
                                                     segments.size() - 2),
                                 "/")
                          : "";
    return std::string(key);
}

class request {
public:
    request() = default;

    request(raw_request rawreq, std::unique_ptr<body> body, ep::user::user user)
        : m_rawreq(std::move(rawreq)),
          m_body(std::move(body)),
          m_authenticated_user(std::move(user)),
          m_bucket_id(get_bucket_id(m_rawreq.path)),
          m_object_key(get_object_key(m_rawreq.path)) {}

    request(const request&) = delete;
    request& operator=(const request&) = delete;
    request(request&&) noexcept = default;
    request& operator=(request&&) noexcept = default;

    verb method() const { return m_rawreq.headers.method(); }

    std::string_view target() const { return m_rawreq.headers.target(); }
    const std::string& path() const { return m_rawreq.path; }

    const std::string& bucket() const { return m_bucket_id; }
    const std::string& object_key() const { return m_object_key; }

    std::string arn() const {
        if (m_object_key.size() != 0)
            return "arn:aws:s3:::" + m_bucket_id + "/" + m_object_key;
        else
            return "arn:aws:s3:::" + m_bucket_id;
    }

    const raw_request& get_raw_request() const noexcept { return m_rawreq; }

    coro<std::size_t> read_body(std::span<char> buffer) {
        return m_body->read(buffer);
    }

    std::vector<boost::asio::const_buffer> get_raw_buffer() const noexcept {
        return m_body->get_raw_buffer();
    }

    boost::asio::ip::tcp::endpoint peer() const { return m_rawreq.peer; }

    /** Payload that was read while reading the request headers.
     */
    std::size_t content_length() const {
        return std::stoul(m_rawreq.headers.at("Content-Length"));
    }

    /**
     * Return value of query parameter specified by `name`. Return
     * `std::nullopt` if parameter is not set.
     */
    std::optional<std::string> query(const std::string& name) const {
        if (auto it = m_rawreq.params.find(name); it != m_rawreq.params.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    const std::map<std::string, std::string>& query_map() const {
        return m_rawreq.params;
    }

    void set_query_params(const std::string& query) {
        boost::urls::url url;
        url.set_encoded_query(query);

        std::map<std::string, std::string> params;
        for (const auto& param : url.params()) {
            params[param.key] = param.value;
        }

        m_rawreq.params = std::move(params);
    }

    bool has_query() const { return !m_rawreq.params.empty(); }

    std::optional<std::string> header(const std::string& name) const {
        if (auto it = m_rawreq.headers.find(name);
            it != m_rawreq.headers.end()) {
            return it->value();
        }
        return {};
    }

    bool keep_alive() const { return m_rawreq.headers.keep_alive(); }

    const user::user& authenticated_user() const {
        return m_authenticated_user;
    }

    const auto& base() const { return m_rawreq.headers; }

private:
    raw_request m_rawreq;
    std::unique_ptr<body> m_body;
    user::user m_authenticated_user;

    std::string m_bucket_id{};
    std::string m_object_key{};
};

std::string get_bucket_id(const std::string& path);
std::string get_object_key(const std::string& path);

/**
 * query string access interface: The following functions allow type-safe
 * access to query parameters, giving the following guarantees:
 *
 * - if (and only in this case) the query parameter is undefined, std::nullopt
 *   is returned
 * - the query string is converted to the target type unless it cannot be
 *   converted in which case an InvalidArgument command_exception is thrown
 */
template <typename return_type = std::string>
std::optional<return_type> query(const request& req, const std::string& name);

template <>
inline std::optional<std::string> query<std::string>(const request& req,
                                                     const std::string& name) {
    return req.query(name);
}

template <>
inline std::optional<std::size_t> query<std::size_t>(const request& req,
                                                     const std::string& name) {
    auto value = req.query(name);
    if (!value) {
        return std::nullopt;
    }

    try {
        return std::stoul(*value);
    } catch (const std::exception&) {
        throw command_exception(http::status::bad_request, "InvalidArgument",
                                "Invalid " + name + ".");
    }
}

template <>
inline std::optional<bool> query<bool>(const request& req,
                                       const std::string& name) {
    auto value = req.query(name);
    if (!value) {
        return std::nullopt;
    }

    return to_bool(*value);
}

inline std::ostream& operator<<(std::ostream& out, const request& req) {
    out << req.base().method_string() << " " << req.base().target() << " ";

    std::string delim;
    for (const auto& field : req.base()) {
        out << delim << field.name_string() << ": " << field.value();
        delim = ", ";
    }

    return out;
}

} // namespace uh::cluster::ep::http
