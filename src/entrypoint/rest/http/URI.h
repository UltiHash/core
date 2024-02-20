#pragma once

#include "http_types.h"
#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/url/decode_view.hpp>
#include <boost/url/url.hpp>
#include <map>
#include <string>

namespace uh::cluster::rest::http {

namespace http = boost::beast::http; // from <boost/beast/http.hpp>
namespace net = boost::asio;

/**
 * Enum representing URI scheme.
 */
enum class scheme { HTTP, HTTPS };

const char* to_string(scheme scheme);
scheme from_string(const char* name);

class URI {
public:
    explicit URI(const http::request_parser<http::empty_body>&);
    ~URI() = default;

    [[nodiscard]] const std::string& get_bucket_id() const;
    [[nodiscard]] const std::string& get_object_key() const;
    [[nodiscard]] bool query_string_exists(const std::string& key) const;
    [[nodiscard]] const std::string&
    get_query_string_value(const std::string& key) const;
    [[nodiscard]] const std::map<std::string, std::string>&
    get_query_parameters() const;
    [[nodiscard]] http_method get_http_method() const;

private:
    void extract_and_set_bucket_id_and_object_key();
    void extract_and_set_query_parameters();

    const http::request_parser<http::empty_body>& m_req;
    scheme m_scheme = scheme::HTTP;
    http_method m_method;
    std::string m_bucket_id{};
    std::string m_object_key{};
    boost::urls::url m_url;
    std::map<std::string, std::string> m_query_parameters;
};

} // namespace uh::cluster::rest::http
