#ifndef CORE_ENTRYPOINT_HTTP_BEAST_UTILS_H
#define CORE_ENTRYPOINT_HTTP_BEAST_UTILS_H

#include "auth_utils.h"
#include "common/types/common_types.h"

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

namespace uh::cluster::ep::http {

namespace beast = boost::beast;
using method = beast::http::verb;

struct partial_parse_result {
    static coro<partial_parse_result> read(boost::asio::ip::tcp::socket& sock);

    std::optional<std::string> optional(const std::string& name);
    std::string require(const std::string& name);

    boost::asio::ip::tcp::socket& socket;
    beast::flat_buffer buffer;

    beast::http::request<beast::http::empty_body> headers;
    std::optional<auth_info> auth;
    std::optional<std::string> signature;
    std::optional<std::string> signing_key;
};

} // namespace uh::cluster::ep::http

#endif
