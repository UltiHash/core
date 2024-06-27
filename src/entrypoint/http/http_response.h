#ifndef ENTRYPOINT_HTTP_HTTP_RESPONSE_H
#define ENTRYPOINT_HTTP_HTTP_RESPONSE_H

#include <boost/beast.hpp>
#include <boost/property_tree/ptree.hpp>

namespace uh::cluster {

namespace http = boost::beast::http;
class http_response {
public:
    http_response();
    http_response(http::status status);

    void set_body(std::string&& body) noexcept;

    void set_etag(const std::string& etag);

    void set_effective_size(std::size_t effective_size);

    void set_original_size(std::size_t original_size);

    const http::response<http::string_body>& get_prepared_response();

    http::response<http::string_body>& base() { return m_res; }
    const http::response<http::string_body>& base() const { return m_res; }

private:
    http::response<http::string_body> m_res;
};

/**
 * Append property_tree as XML to the response.
 */
http_response& operator<<(http_response& res,
                          const boost::property_tree::ptree& pt);

std::ostream& operator<<(std::ostream& out, const http_response& res);
} // namespace uh::cluster

#endif
