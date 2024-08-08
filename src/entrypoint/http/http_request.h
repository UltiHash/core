#ifndef ENTRYPOINT_HTTP_HTTP_REQUEST_H
#define ENTRYPOINT_HTTP_HTTP_REQUEST_H

#include "common/coroutines/context.h"
#include "common/types/common_types.h"
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <map>
#include <span>

namespace uh::cluster {
namespace http = boost::beast::http; // from <boost/beast/http.hpp>
typedef http::verb method;

class transport_decoder {
public:
    virtual ~transport_decoder() = default;
    virtual coro<std::size_t> read(std::span<char> dest) = 0;
};

class http_request {
public:
    static coro<std::unique_ptr<http_request>>
    create(context& ctx, boost::asio::ip::tcp::socket& s);

    [[nodiscard]] http::verb method() const;

    const std::string& bucket() const;
    const std::string& object_key() const;

    coro<std::size_t> read_body(std::span<char> buffer);

    [[nodiscard]] boost::asio::ip::tcp::socket& socket() const {
        return m_stream;
    }

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

    bool has_query() const;

    std::optional<std::string> header(const std::string& name) const;

    bool keep_alive() const { return m_req.keep_alive(); }

    context& m_ctx;

private:
    friend std::ostream& operator<<(std::ostream& out, const http_request& req);

    http_request(context& ctx, boost::asio::ip::tcp::socket& stream,
                 http::request_parser<http::empty_body>::value_type&& req,
                 boost::beast::flat_buffer&& initial);

    boost::asio::ip::tcp::socket& m_stream;
    http::request_parser<http::empty_body>::value_type m_req;
    std::unique_ptr<transport_decoder> m_decoder;

    std::string m_bucket_id{};
    std::string m_object_key{};
    std::map<std::string, std::string> m_params;
};

std::ostream& operator<<(std::ostream& out, const http_request& req);

} // namespace uh::cluster

#endif
