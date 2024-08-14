#ifndef ENTRYPOINT_HTTP_HTTP_RESPONSE_H
#define ENTRYPOINT_HTTP_HTTP_RESPONSE_H

#include <boost/beast.hpp>
#include <boost/property_tree/ptree.hpp>
#include <common/types/common_types.h>

namespace uh::cluster {

class body {
public:
    virtual ~body() = default;

    /**
     * Return remaining length of body if available.
     */
    virtual std::optional<std::size_t> length() const = 0;

    /**
     * Fill the provided span with the next bytes of the body, return number of
     * bytes written.
     */
    virtual coro<std::size_t> read(std::span<char>) = 0;
};

class string_body : public body {
public:
    string_body(std::string&& body);

    std::optional<std::size_t> length() const override;
    coro<std::size_t> read(std::span<char>) override;

private:
    std::string m_body;
};

namespace http = boost::beast::http;
class http_response {
public:
    http_response();
    http_response(http::status status);

    void set_body(std::unique_ptr<uh::cluster::body>&& body) noexcept;

    void set_effective_size(std::size_t effective_size);
    void set_original_size(std::size_t original_size);

    http::response<http::empty_body>& base() { return m_res; }
    const http::response<http::empty_body>& base() const { return m_res; }

    /**
     * Set value for header `header` to `value` or remove header in case
     * `value == std::nullopt`.
     */
    void set(const std::string& header, std::optional<std::string> value);
    void set(const std::string& header, std::optional<std::size_t> value);

    uh::cluster::body& body() { return *m_body; }

private:
    http::response<http::empty_body> m_res;
    std::unique_ptr<uh::cluster::body> m_body;
};

/**
 * Append property_tree as XML to the response.
 */
http_response& operator<<(http_response& res,
                          const boost::property_tree::ptree& pt);

std::ostream& operator<<(std::ostream& out, const http_response& res);

coro<void> write(boost::asio::ip::tcp::socket& out, http_response&& res);

} // namespace uh::cluster

#endif
