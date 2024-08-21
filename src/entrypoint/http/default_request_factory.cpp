#include "default_request_factory.h"

#include "chunked_body.h"
#include "raw_body.h"

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

using namespace boost;
using namespace boost::asio;

namespace uh::cluster::ep::http {

namespace http = boost::beast::http; // from <boost/beast/http.hpp>

namespace {

std::unique_ptr<ep::http::body>
make_body(const beast::http::request<http::empty_body>& req,
          asio::ip::tcp::socket& sock, beast::flat_buffer&& initial) {

    /* Amazon will upload data using chunked transfer without explicitly setting
     * the `Transfer-Encoding` header for signed data. This also prevents us
     * from using beasts HTTP parser for decoding the transfer encoding.
     *
     * (see
     * https://docs.aws.amazon.com/AmazonS3/latest/API/sigv4-auth-using-authorization-header.html#sigv4-auth-header-overview)
     */
    auto content_sha = req.base().find("x-amz-content-sha256");
    if (content_sha != req.base().end() &&
        (content_sha->value() == "STREAMING-UNSIGNED-PAYLOAD-TRAILER" ||
         content_sha->value() == "STREAMING-AWS4-HMAC-SHA256-PAYLOAD" ||
         content_sha->value() == "STREAMING-AWS4-HMAC-SHA256-PAYLOAD-TRAILER" ||
         content_sha->value() == "STREAMING-AWS4-ECDSA-P256-SHA256-PAYLOAD" ||
         content_sha->value() ==
             "STREAMING-AWS4-ECDSA-P256-SHA256-PAYLOAD-TRAILER")) {
        return std::make_unique<chunked_body>(sock, std::move(initial));
    }

    std::size_t length = 0ull;

    auto content_length = req.base().find("content-length");
    if (content_length != req.base().end()) {
        length = std::stoul(content_length->value());
    }

    return std::make_unique<raw_body>(sock, std::move(initial), length);
}

} // namespace

coro<std::unique_ptr<http_request>>
default_request_factory::create(ip::tcp::socket& sock) {

    http::request_parser<http::empty_body> parser;
    boost::beast::flat_buffer buffer;
    parser.body_limit((std::numeric_limits<std::uint64_t>::max)());

    co_await beast::http::async_read_header(sock, buffer, parser,
                                            asio::use_awaitable);

    auto req = std::move(parser.get());
    if (req.base().version() != 11) {
        throw std::runtime_error(
            "bad http version. support exists only for HTTP 1.1.\n");
    }

    auto body = make_body(req, sock, std::move(buffer));

    co_return std::unique_ptr<http_request>(new http_request(
        std::move(req), std::move(body), sock.remote_endpoint()));
}

} // namespace uh::cluster::ep::http
