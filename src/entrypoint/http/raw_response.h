#pragma once

#include <common/types/common_types.h>
#include <entrypoint/http/header.h>

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

#include <optional>
#include <string>
#include <vector>

namespace uh::cluster::ep::http {

namespace beast = boost::beast;
using status = beast::http::status;

class raw_response
    : public header<beast::http::response<beast::http::empty_body>> {
public:
    static coro<raw_response> read(boost::asio::ip::tcp::socket& sock);
    static raw_response
    from_string(beast::http::response<beast::http::empty_body> header,
                std::vector<char>&& buffer, size_t header_length);
};

} // namespace uh::cluster::ep::http
