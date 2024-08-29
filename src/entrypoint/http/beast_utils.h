#ifndef CORE_ENTRYPOINT_HTTP_BEAST_UTILS_H
#define CORE_ENTRYPOINT_HTTP_BEAST_UTILS_H

#include "auth_utils.h"
#include "common/types/common_types.h"

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/url/url.hpp>

namespace uh::cluster::ep::http {

namespace beast = boost::beast;
using method = beast::http::verb;
using status = beast::http::status;

struct partial_parse_result {
    static coro<partial_parse_result> read(boost::asio::ip::tcp::socket& sock);

    void set_secret(const std::string& key);

    std::optional<std::string> optional(const std::string& name);
    std::string require(const std::string& name);

    boost::asio::ip::tcp::socket& socket;
    beast::flat_buffer buffer;

    beast::http::request<beast::http::empty_body> headers;
    std::optional<auth_info> auth;
    std::optional<std::string> signature;
    std::optional<std::string> signing_key;

    boost::asio::ip::tcp::endpoint peer;
};

struct url_parsing_result {
    std::map<std::string, std::string> params;
    std::string path;
    std::string encoded_path;
    std::string bucket;
    std::string object;
};

url_parsing_result parse_request_target(const std::string& target);

/**
 * Return bucket and object key.
 */
std::tuple<std::string, std::string>
extract_bucket_and_object(boost::urls::url url);

} // namespace uh::cluster::ep::http

#endif
