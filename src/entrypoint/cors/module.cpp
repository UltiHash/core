#include "module.h"

#include "parser.h"
#include <common/utils/strings.h>
#include <ranges>

namespace uh::cluster::ep::cors {

namespace {

http::response options_response(const http::request& r, std::string origin,
                                cors::info info) {

    auto response = http::response(http::status::no_content);
    response.set("Access-Control-Allow-Origin", std::move(origin));

    if (auto acrh = r.header("Access-Control-Request-Headers"); acrh) {
        auto rheaders = split<std::set<std::string>>(*acrh, ',');

        std::set<std::string> intersection;
        std::set_intersection(
            rheaders.begin(), rheaders.end(), info.headers.begin(),
            info.headers.end(),
            std::inserter(intersection, intersection.begin()));

        std::string headers = join(intersection, ",");
        if (!headers.empty()) {
            response.set("Access-Control-Allow-Headers", std::move(headers));
        }
    }

    response.set("Access-Control-Max-Age", info.max_age_seconds);

    auto verb_str = [](http::verb v) { return to_string(v); };
    std::string methods =
        join(info.methods | std::views::transform(verb_str), ",");
    if (!methods.empty()) {
        response.set("Access-Control-Allow-Methods", std::move(methods));
    }

    return response;
}

} // namespace

module::module(directory& dir) :m_directory(dir) {}

coro<result> module::check(const http::request& request) const {
    auto origin = request.header("origin");
    if (!origin) {
        co_return result{};
    }

    auto config = co_await m_directory.get_bucket_cors(request.bucket());
    if (!config) {
        co_return result{
            .response = error_response(
                http::status::forbidden, "Forbidden",
                "CORS Response: CORS is not enabled for this bucket")};
    }

    auto infos = parser::parse(*config);
    auto origin_info = infos.find(*origin);
    if (origin_info == infos.end()) {
        origin_info = infos.find("*");
    }

    if (origin_info == infos.end()) {
        co_return result{
            .response = error_response(
                http::status::forbidden, "Forbidden",
                "CORS Response: This CORS request is not allowed")};
    }

    const auto& info = origin_info->second;

    if (request.method() == http::verb::options) {
        co_return result{
            .response = options_response(request, std::move(*origin), info)};
    }

    if (!info.methods.contains(request.method())) {
        co_return result{
            .response = error_response(
                http::status::forbidden, "Forbidden",
                "CORS Response: This CORS request is not allowed")};
    }

    std::map<std::string, std::string> headers;
    headers["Access-Control-Allow-Origin"] = *origin;
    if (info.exposed_headers) {
        headers["Access-Control-Expose-Headers"] = *info.exposed_headers;
    }

    co_return result{.headers = std::move(headers)};
}

} // namespace uh::cluster::ep::cors
