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
        // TODO when configured in CORS-Rule:
        // response.set("Access-Control-Allow-Headers", "");
        // TODO when configured in CORS-Rule:
        // response.set("Access-Control-Max-Age", "");

        std::string allowed_methods;
        if (origin_info->second.allowed_get) {
            allowed_methods += "GET, ";
        }
        if (origin_info->second.allowed_post) {
            allowed_methods += "POST, ";
        }
        if (origin_info->second.allowed_put) {
            allowed_methods += "PUT, ";
        }
        if (origin_info->second.allowed_head) {
            allowed_methods += "HEAD, ";
        }
        if (origin_info->second.allowed_delete) {
            allowed_methods += "DELETE, ";
        }

        if (!allowed_methods.empty()) {
            allowed_methods.erase(allowed_methods.size() - 2);
        }

        response.set("Access-Control-Allow-Methods", allowed_methods);

        co_return result{.response = std::move(response)};
    }

    auto infos = parser::parse(*config);
    auto origin_info = infos.find(*origin);
    if (origin_info == infos.end()) {
        co_return result{
            .response = make_response(command_exception(
                http::status::forbidden, "Forbidden",
                "CORS Response: This CORS request is not allowed"))};
    }

    if (request.method() == http::verb::get &&
        !origin_info->second.allowed_get) {
        co_return result{
            .response = make_response(command_exception(
                http::status::forbidden, "Forbidden",
                "CORS Response: This CORS request is not allowed"))};
    }

    if (request.method() == http::verb::put &&
        !origin_info->second.allowed_put) {
        co_return result{
            .response = make_response(command_exception(
                http::status::forbidden, "Forbidden",
                "CORS Response: This CORS request is not allowed"))};
    }

    if (request.method() == http::verb::head &&
        !origin_info->second.allowed_head) {
        co_return result{
            .response = make_response(command_exception(
                http::status::forbidden, "Forbidden",
                "CORS Response: This CORS request is not allowed"))};
    }

    if (request.method() == http::verb::post &&
        !origin_info->second.allowed_post) {
        co_return result{
            .response = make_response(command_exception(
                http::status::forbidden, "Forbidden",
                "CORS Response: This CORS request is not allowed"))};
    }

    if (request.method() == http::verb::delete_ &&
        !origin_info->second.allowed_delete) {
        co_return result{
            .response = make_response(command_exception(
                http::status::forbidden, "Forbidden",
                "CORS Response: This CORS request is not allowed"))};
    }

    co_return result{
        .headers = std::map<std::string, std::string>{
            {"Access-Control-Allow-Origin", *origin},
            // { "Access-Control-Allow-Credentials", *origin }, TODO enable when
            // configured in CORS-rule { "Access-Control-Expose-Headers",
            // headers } TODO enable when configured in CORS-Rule
        }};
}

} // namespace uh::cluster::ep::cors
