#ifndef ENTRYPOINT_HTTP_HTTP_RESPONSE_H
#define ENTRYPOINT_HTTP_HTTP_RESPONSE_H

#include "boost/beast.hpp"

namespace uh::cluster {

namespace http = boost::beast::http;
class http_response {
public:
    http_response() = default;

    void set_body(std::string&& body) noexcept;

    void set_etag(const std::string& etag);

    void set_effective_size(std::size_t effective_size);

    void set_space_savings(double space_savings);

    void set_bandwidth(double bandwidth);

    void set_original_size(std::size_t original_size);

    const http::response<http::string_body>& get_prepared_response();

private:
    http::response<http::string_body> m_res{
        http::response<http::string_body>{http::status::ok, 11}};
};

} // namespace uh::cluster

#endif
