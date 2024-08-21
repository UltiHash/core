#ifndef ENTRYPOINT_HTTP_HTTP_REQUEST_H
#define ENTRYPOINT_HTTP_HTTP_REQUEST_H

#include "command_exception.h"
#include "common/coroutines/context.h"
#include "common/types/common_types.h"
#include "common/utils/strings.h"
#include "entrypoint/http/body.h"

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <map>
#include <span>

namespace uh::cluster {
namespace http = boost::beast::http; // from <boost/beast/http.hpp>
typedef http::verb method;

struct read_request_result {
    boost::beast::http::request<boost::beast::http::empty_body> body;
    boost::beast::flat_buffer buffer;
};

coro<read_request_result>
read_beast_request(boost::asio::ip::tcp::socket& sock);

class http_request {
public:
    http_request(boost::beast::http::request<http::empty_body>&& req,
                 std::unique_ptr<ep::http::body> body,
                 boost::asio::ip::tcp::endpoint peer);

    [[nodiscard]] http::verb method() const;

    std::string_view target() const;
    const std::string& path() const;
    const std::string& query() const;

    const std::string& bucket() const;
    const std::string& object_key() const;

    coro<std::size_t> read_body(std::span<char> buffer);

    boost::asio::ip::tcp::endpoint peer() const { return m_peer; }

    /** Payload that was read while reading the request headers.
     */
    std::size_t content_length() const {
        return std::stoul(m_req.at("Content-Length"));
    }

    /**
     * Return value of query parameter specified by `name`. Return
     * `std::nullopt` if parameter is not set.
     */
    std::optional<std::string> query(const std::string& name) const;
    const std::map<std::string, std::string>& query_map() const;

    const boost::beast::http::fields& header() const;

    bool has_query() const;

    std::optional<std::string> header(const std::string& name) const;

    bool keep_alive() const { return m_req.keep_alive(); }

    const uh::cluster::context& context() const;
    uh::cluster::context& context();

private:
    friend std::ostream& operator<<(std::ostream& out, const http_request& req);

    boost::beast::http::request<http::empty_body> m_req;
    std::unique_ptr<ep::http::body> m_body;
    boost::asio::ip::tcp::endpoint m_peer;

    std::string m_bucket_id{};
    std::string m_object_key{};
    std::map<std::string, std::string> m_params;
    std::string m_path;
    std::string m_query;

    uh::cluster::context m_ctx;
};

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
std::optional<return_type> query(const http_request& req,
                                 const std::string& name);

template <>
inline std::optional<std::string> query<std::string>(const http_request& req,
                                                     const std::string& name) {
    return req.query(name);
}

template <>
inline std::optional<std::size_t> query<std::size_t>(const http_request& req,
                                                     const std::string& name) {
    auto value = req.query(name);
    if (!value) {
        return std::nullopt;
    }

    try {
        return std::stoul(*value);
    } catch (const std::exception&) {
        throw command_exception(http::status::bad_request, "InvalidArgument",
                                "invalid " + name);
    }
}

template <>
inline std::optional<bool> query<bool>(const http_request& req,
                                       const std::string& name) {
    auto value = req.query(name);
    if (!value) {
        return std::nullopt;
    }

    return to_bool(*value);
}

std::ostream& operator<<(std::ostream& out, const http_request& req);

} // namespace uh::cluster

#endif
