#include "beast_utils.h"

#include "command_exception.h"
#include "common/crypto/hash.h"
#include "common/crypto/hmac.h"
#include "common/telemetry/log.h"
#include "common/utils/strings.h"

#include <boost/algorithm/string.hpp>

using namespace boost;

namespace uh::cluster::ep::http {

namespace {

bool include_header(const std::string& name,
                    const std::set<std::string_view>& included) {
    return name == "host" || name == "content-md5" ||
           name.starts_with("x-amz-") || included.contains(name);
}

std::string make_canonical_request(partial_parse_result& req) {

    auto url = parse_request_target(req.headers.target());

    // for details, see
    // https://docs.aws.amazon.com/AmazonS3/latest/API/sig-v4-header-based-auth.html
    std::set<std::string> canonical_query_set;
    for (const auto& field : url.params) {
        canonical_query_set.emplace(url_encode(field.first) + "=" +
                                    url_encode(field.second));
    }

    std::string canonical_query = algorithm::join(canonical_query_set, "&");

    std::map<std::string, std::string> canonical_headers_map;
    for (const auto& header : req.headers) {
        auto name = lowercase(header.name_string());

        if (!include_header(name, req.auth->signed_headers)) {
            continue;
        }

        canonical_headers_map[std::move(name)] = trim(header.value());
    }

    std::string canonical_headers;
    std::string signed_header_names;
    bool first = true;

    for (const auto& header : canonical_headers_map) {
        if (!first) {
            signed_header_names += ";";
        }

        canonical_headers += header.first + ":" + header.second + "\n";
        signed_header_names += header.first;
        first = false;
    }

    return std::string(req.headers.method_string()) + "\n" + url.encoded_path +
           "\n" + canonical_query + "\n" + canonical_headers + "\n" +
           signed_header_names + "\n" + req.require("x-amz-content-sha256");
}

std::string request_signature(partial_parse_result& req) {

    auto canonical_request = make_canonical_request(req);
    LOG_DEBUG() << req.peer << ": canonical request: " << canonical_request;

    std::stringstream string_to_sign;
    string_to_sign << "AWS4-HMAC-SHA256\n"
                   << req.require("x-amz-date") << "\n"
                   << req.auth->date << "/" << req.auth->region << "/"
                   << req.auth->service << "/aws4_request\n"
                   << sha256::from_string(canonical_request);

    LOG_DEBUG() << req.peer << ": string to sign: " << string_to_sign.str();

    return to_hex(
        hmac_sha256::from_string(*req.signing_key, string_to_sign.str()));
}

} // namespace

coro<partial_parse_result>
partial_parse_result::read(asio::ip::tcp::socket& sock) {

    beast::http::request_parser<beast::http::empty_body> parser;
    beast::flat_buffer buffer;
    parser.body_limit((std::numeric_limits<std::uint64_t>::max)());

    co_await beast::http::async_read_header(sock, buffer, parser,
                                            asio::use_awaitable);

    auto req = std::move(parser.get());
    if (req.base().version() != 11) {
        throw std::runtime_error(
            "bad http version. support exists only for HTTP 1.1.\n");
    }

    auto rv = partial_parse_result{sock, std::move(buffer), std::move(req)};
    rv.peer = sock.remote_endpoint();

    if (auto authorization = rv.optional("authorization"); authorization) {
        try {
            rv.auth = auth_info(*authorization);
        } catch (const std::exception& e) {
            LOG_INFO() << rv.peer
                       << ": error parsing authorization header: " << e.what();
            throw command_exception(
                status::forbidden, "AuthorizationHeaderMalformed",
                "The authorization header that you provided is not valid.");
        }
    }

    co_return rv;
}

void partial_parse_result::set_secret(const std::string& key) {
    signing_key = auth->signing_key(key);
    signature = request_signature(*this);

    if (*signature != auth->signature) {
        LOG_INFO() << peer << ": access denied: signature mismatch";
        throw command_exception(status::forbidden, "AccessDenied",
                                "Access Denied");
    }
}

std::optional<std::string>
partial_parse_result::optional(const std::string& name) {

    if (auto header = headers.find(name); header != headers.end()) {
        return header->value();
    }

    return {};
}

std::string partial_parse_result::require(const std::string& name) {

    auto header = headers.find(name);
    if (header == headers.end()) {
        throw std::runtime_error(name + " not found");
    }

    return header->value();
}

std::tuple<std::string, std::string>
extract_bucket_and_object(boost::urls::url url) {
    std::string bucket_id;
    std::string object_key;

    for (const auto& seg : url.segments()) {
        if (bucket_id.empty())
            bucket_id = seg;
        else
            object_key += seg + '/';
    }

    if (!object_key.empty())
        object_key.pop_back();

    return std::make_tuple(bucket_id, object_key);
}

url_parsing_result parse_request_target(const std::string& target) {
    auto query_index = target.find('?');

    boost::urls::url url;
    if (query_index != std::string::npos) {
        url.set_encoded_path(target.substr(0, query_index));
        url.set_encoded_query(target.substr(query_index + 1));
    } else {
        url.set_encoded_path(target);
    }

    auto keys = extract_bucket_and_object(url);

    url_parsing_result rv;
    rv.path = url.path();
    rv.encoded_path = url.encoded_path();

    for (const auto& param : url.params()) {
        rv.params[param.key] = param.value;
    }

    rv.bucket = std::move(std::get<0>(keys));
    rv.object = std::move(std::get<1>(keys));
    return rv;
}

} // namespace uh::cluster::ep::http
