#ifndef ENTRYPOINT_HTTP_HTTP_RESPONSE_H
#define ENTRYPOINT_HTTP_HTTP_RESPONSE_H

#include "boost/beast.hpp"

namespace uh::cluster {

namespace http = boost::beast::http;
class http_response {
public:
    void set_body(std::string&& body) noexcept;

    void set_etag(const std::string& etag);

    void set_effective_size(std::size_t effective_size);

    void set_original_size(std::size_t original_size);

    const http::response<http::string_body>& get_prepared_response();

    http::response<http::string_body>& base() { return m_res; }
    const http::response<http::string_body>& base() const { return m_res; }

private:
    http::response<http::string_body> m_res{
        http::response<http::string_body>{http::status::ok, 11}};
};

std::ostream& operator<<(std::ostream& out, const http_response& res);
} // namespace uh::cluster

#endif
