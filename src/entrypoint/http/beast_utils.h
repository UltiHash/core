#ifndef CORE_ENTRYPOINT_HTTP_BEAST_UTILS_H
#define CORE_ENTRYPOINT_HTTP_BEAST_UTILS_H

#include "common/types/common_types.h"

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

namespace uh::cluster::ep::http {

namespace beast = boost::beast;
using method = beast::http::verb;

struct read_request_result {
    beast::http::request<beast::http::empty_body> headers;
    beast::flat_buffer buffer;
};

coro<read_request_result>
read_beast_request(boost::asio::ip::tcp::socket& sock);

std::optional<std::string>
optional(const beast::http::request<beast::http::empty_body>& headers,
         const std::string& name);

std::string
require(const beast::http::request<beast::http::empty_body>& headers,
        const std::string& name);

} // namespace uh::cluster::ep::http

#endif
