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

class request {
public:
    request() = default;

    request(raw_request rawreq, std::unique_ptr<body> body,
            ep::user::user user);

    request(const request&) = delete;
    request& operator=(const request&) = delete;
    request(request&&) noexcept = default;
    request& operator=(request&&) noexcept = default;

    verb method() const;

    std::string_view target() const;
    const std::string& path() const;

    const std::string& bucket() const;
    const std::string& object_key() const;

    std::string arn() const;

    const raw_request& get_raw_request() const noexcept;

    coro<std::size_t> read_body(std::span<char> buffer);

    std::vector<boost::asio::const_buffer> get_raw_buffer() const;

    boost::asio::ip::tcp::endpoint peer() const;

    /** Payload that was read while reading the request headers.
     */
    std::size_t content_length() const;

    /**
     * Return value of query parameter specified by `name`. Return
     * `std::nullopt` if parameter is not set.
     */
    std::optional<std::string> query(const std::string& name) const;

    const std::map<std::string, std::string>& query_map() const;

    void set_query_params(const std::string& query);

    bool has_query() const;

    std::optional<std::string> header(const std::string& name) const;

    bool keep_alive() const;

    const user::user& authenticated_user() const;

    const beast::http::request<beast::http::empty_body>& base() const;

private:
    friend std::ostream& operator<<(std::ostream& out, const request& req);
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

std::ostream& operator<<(std::ostream& out, const request& req);

} // namespace uh::cluster::ep::http
