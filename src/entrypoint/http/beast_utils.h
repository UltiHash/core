#ifndef CORE_ENTRYPOINT_HTTP_BEAST_UTILS_H
#define CORE_ENTRYPOINT_HTTP_BEAST_UTILS_H

#include "common/types/common_types.h"
#include "entrypoint/http/auth_utils.h"

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/url/url.hpp>

#include <map>
#include <optional>
#include <string>
#include <tuple>

namespace uh::cluster::ep::http {

namespace beast = boost::beast;
using verb = beast::http::verb;
using status = beast::http::status;

struct partial_parse_result {
    static coro<partial_parse_result> read(boost::asio::ip::tcp::socket& sock);

    std::optional<std::string> optional(const std::string& name);
    std::string require(const std::string& name);

    boost::asio::ip::tcp::socket& socket;
    beast::flat_buffer buffer;

    beast::http::request<beast::http::empty_body> headers;
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

std::map<std::string_view, std::string_view>
parse_values_string(std::string_view values, char pair_separator = ',',
                    char field_separator = '=');

} // namespace uh::cluster::ep::http

#endif
