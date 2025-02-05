#include "module.h"

#include "parser.h"
#include <common/utils/strings.h>
#include <ranges>

namespace uh::cluster::ep::cors {

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
    auto& info = origin_info->second;

    if (request.method() == http::verb::options) {
        auto response = http::response(http::status::no_content);
        response.set("Access-Control-Allow-Origin", *origin);
        // TODO check with AWS documentation:
        // response.set("Access-Control-Allow-Credentials", "");
        if (auto request_headers =
                request.header("Access-Control-Request-Headers");
            request_headers) {
            auto headers = split<std::set<std::string>>(*request_headers, ',');

            std::set<std::string> intersection;
            std::set_intersection(
                headers.begin(), headers.end(), info.allowed_headers.begin(),
                info.allowed_headers.end(),
                std::inserter(intersection, intersection.begin()));

            std::string allow_headers = join(intersection, ",");
            if (!allow_headers.empty()) {
                response.set("Access-Control-Allow-Headers",
                             std::move(allow_headers));
            }
        }

        response.set("Access-Control-Max-Age", info.max_age_seconds);

        auto verb_str = [](http::verb v) { return to_string(v); };
        std::string allowed_methods =
            join(info.allowed_methods | std::views::transform(verb_str), ",");
        if (!allowed_methods.empty()) {
            response.set("Access-Control-Allow-Methods",
                         std::move(allowed_methods));
        }

        co_return result{.response = std::move(response)};
    }

    if (!info.allowed_methods.contains(request.method())) {
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
