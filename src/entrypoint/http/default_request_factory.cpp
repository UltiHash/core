#include "default_request_factory.h"

#include "chunked_body.h"
#include "raw_body.h"

#include "beast_utils.h"

using namespace boost;
using namespace boost::asio;

namespace uh::cluster::ep::http {

namespace {

std::unique_ptr<ep::http::body> make_body(partial_parse_result& req) {

    /* Amazon will upload data using chunked transfer without explicitly setting
     * the `Transfer-Encoding` header for signed data. This also prevents us
     * from using beasts HTTP parser for decoding the transfer encoding.
     *
     * (see
     * https://docs.aws.amazon.com/AmazonS3/latest/API/sigv4-auth-using-authorization-header.html#sigv4-auth-header-overview)
     */
    auto content_sha = req.optional("x-amz-content-sha256");
    if (content_sha &&
        (*content_sha == "STREAMING-UNSIGNED-PAYLOAD-TRAILER" ||
         *content_sha == "STREAMING-AWS4-HMAC-SHA256-PAYLOAD" ||
         *content_sha == "STREAMING-AWS4-HMAC-SHA256-PAYLOAD-TRAILER" ||
         *content_sha == "STREAMING-AWS4-ECDSA-P256-SHA256-PAYLOAD" ||
         *content_sha == "STREAMING-AWS4-ECDSA-P256-SHA256-PAYLOAD-TRAILER")) {
        return std::make_unique<chunked_body>(req);
    }

    std::size_t length =
        std::stoul(req.optional("content-length").value_or("0"));
    return std::make_unique<raw_body>(req, length);
}

} // namespace

coro<std::unique_ptr<http_request>>
default_request_factory::create(ip::tcp::socket& sock) {

    auto req = co_await partial_parse_result::read(sock);
    auto body = make_body(req);

    co_return std::unique_ptr<http_request>(new http_request(
        std::move(req.headers), std::move(body), req.socket.remote_endpoint()));
}

} // namespace uh::cluster::ep::http
