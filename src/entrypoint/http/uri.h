#ifndef ENTRYPOINT_HTTP_URI_H
#define ENTRYPOINT_HTTP_URI_H

#include "boost/url/decode_view.hpp"
#include "boost/url/url.hpp"
#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <map>
#include <string>

namespace uh::cluster {

namespace http = boost::beast::http; // from <boost/beast/http.hpp>
typedef http::verb method;
typedef boost::urls::url url;

/**
 * Enum representing URI scheme.
 */
enum class scheme { HTTP, HTTPS };

class uri {
public:
    explicit uri(const http::request_parser<http::empty_body>::value_type&);
    ~uri() = default;

    const std::string& get_bucket_id() const;
    const std::string& get_object_key() const;
    bool query_string_exists(const std::string& key) const;
    const std::string& get_query_string_value(const std::string& key) const;
    const std::map<std::string, std::string>& get_query_parameters() const;
    method get_method() const;

private:
    void extract_bucket_and_object();
    void extract_query_parameters();

    scheme m_scheme = scheme::HTTP;
    method m_method;
    std::string m_bucket_id{};
    std::string m_object_key{};
    url m_url;
    std::map<std::string, std::string> m_query_parameters;
};

} // namespace uh::cluster

#endif
