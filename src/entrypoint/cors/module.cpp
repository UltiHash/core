#include "module.h"

#include "parser.h"

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
            .response = make_response(command_exception(
                http::status::forbidden, "Forbidden",
                "CORS Response: CORS is not enabled for this bucket"))};
    }

    if (request.method() == http::verb::options) {
        auto infos = parser::parse(*config);
        auto origin_info = infos.find(*origin);
        // TODO check for asterisk origin
        if (origin_info == infos.end()) {
            co_return result{
                .response = make_response(command_exception(
                    http::status::forbidden, "Forbidden",
                    "CORS Response: This CORS request is not allowed"))};
        }

        auto response = http::response(http::status::no_content);
        response.set("Access-Control-Allow-Origin", *origin);
        // TODO check with AWS documentation:
        // response.set("Access-Control-Allow-Credentials", "");
        if (auto request_headers =
                request.header("Access-Control-Request-Headers");
            request_headers) {
            auto headers =
                split<std::vector<std::string>>(*request_headers, ',');

            std::string allow_headers;
            for (const auto& header : headers) {
                if (origin_info->second.allowed_headers.contains(header)) {
                    allow_headers += header + ",";
                }
            }

            if (!allow_headers.empty()) {
                allow_headers.erase(allow_headers.size() - 1);
                response.set("Access-Control-Allow-Headers",
                             std::move(allow_headers));
            }
        }

        response.set("Access-Control-Max-Age",
                     origin_info->second.max_age_seconds);

        std::string allowed_methods;
        for (auto method : origin_info->second.allowed_methods) {
            allowed_methods += to_string(method) + ", ";
        }

        if (!allowed_methods.empty()) {
            allowed_methods.erase(allowed_methods.size() - 2);
            response.set("Access-Control-Allow-Methods",
                         std::move(allowed_methods));
        }

        co_return result{.response = std::move(response)};
    }

    auto infos = parser::parse(*config);
    auto origin_info = infos.find(*origin);
    // TODO check for asterisk origin
    if (origin_info == infos.end()) {
        co_return result{
            .response = make_response(command_exception(
                http::status::forbidden, "Forbidden",
                "CORS Response: This CORS request is not allowed"))};
    }

    if (!origin_info->second.allowed_methods.contains(request.method())) {
        co_return result{
            .response = make_response(command_exception(
                http::status::forbidden, "Forbidden",
                "CORS Response: This CORS request is not allowed"))};
    }

    std::map<std::string, std::string> headers;
    headers["Access-Control-Allow-Origin"] = *origin;
    if (origin_info->second.exposed_headers) {
        headers["Access-Control-Expose-Headers"] =
            *origin_info->second.exposed_headers;
    }

    co_return result{.headers = std::move(headers)};
}

} // namespace uh::cluster::ep::cors
