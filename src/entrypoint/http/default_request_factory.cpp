#include "default_request_factory.h"

#include "chunked_body.h"
#include "raw_body.h"

#include "beast_utils.h"

using namespace boost;
using namespace boost::asio;

namespace uh::cluster::ep::http {

namespace {

std::unique_ptr<ep::http::body> make_body(partial_parse_result& req) {

    bool chunked =
        req.optional("content-encoding").value_or("none") == "aws-chunked" ||
        req.optional("transfer-encoding").value_or("none") == "chunked";

    auto content_sha = req.optional("x-amz-content-sha256");
    if ((content_sha.has_value() &&
         (content_sha == "STREAMING-AWS4-HMAC-SHA256-PAYLOAD" ||
          content_sha == "STREAMING-AWS4-ECDSA-P256-SHA256-PAYLOAD")) ||
        chunked) {
        return std::make_unique<chunked_body>(req);
    }

    if (content_sha &&
        (content_sha == "STREAMING-UNSIGNED-PAYLOAD-TRAILER" ||
         content_sha == "STREAMING-AWS4-HMAC-SHA256-PAYLOAD-TRAILER" ||
         content_sha == "STREAMING-AWS4-ECDSA-P256-SHA256-PAYLOAD-TRAILER")) {
        return std::make_unique<chunked_body>(
            req, chunked_body::trailing_headers::read);
    }

    auto length = std::stoul(req.optional("content-length").value_or("0"));
    return std::make_unique<raw_body>(req, length);
}

} // namespace

coro<std::unique_ptr<request>>
default_request_factory::create(ip::tcp::socket& sock) {

    auto req = co_await partial_parse_result::read(sock);
    auto body = make_body(req);

    co_return std::make_unique<request>(req, std::move(body));
}

} // namespace uh::cluster::ep::http
