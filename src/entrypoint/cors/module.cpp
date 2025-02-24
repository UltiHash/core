#include "module.h"

#include "parser.h"
#include <common/utils/strings.h>
#include <ranges>

namespace uh::cluster::ep::cors {

namespace {

bool equals_wildcard_single(std::string_view pattern, std::string_view str) {
    auto pos = pattern.find('*');

    if (pos == std::string::npos) {
        return str == pattern;
    }

    if (pos > 0 && !str.starts_with(pattern.substr(0, pos - 1))) {
        return false;
    }

    return str.ends_with(pattern.substr(pos + 1));
}

std::optional<std::reference_wrapper<const info>>
find_info(const std::vector<info>& rules, const std::string& origin,
          http::verb method) {
    for (const auto& info : rules) {
        if (info.origin != origin && info.origin != "*") {
            continue;
        }

        if (!info.methods.contains(method)) {
            continue;
        }

        return std::cref(info);
    }

    return std::nullopt;
}

} // namespace

module::module(const config& cfg, directory& dir) :m_directory(dir),
    m_info_cache(cfg.cache_retention) {}

coro<result> module::check(const http::request& request) const {
    auto origin = request.header("origin");
    if (!origin) {
        co_return result{};
    }

    if (request.method() == http::verb::options) {
        co_return co_await preflight(request);
    } else {
        co_return co_await flight(request);
    }
}

coro<result> module::preflight(const http::request& r) const {

    auto rules = co_await get_info(r.bucket());

    auto req_method = r.header("Access-Control-Request-Method");
    if (!req_method) {
        co_return result{
            .response = error_response(
                http::status::forbidden, "Forbidden",
                "CORS Response: This CORS request is not allowed")};
    }

    auto method = boost::beast::http::string_to_verb(*req_method);

    auto origin = *r.header("origin");
    auto origin_info = find_info(*rules, origin, method);
    if (!origin_info) {
        co_return result{
            .response = error_response(
                http::status::forbidden, "Forbidden",
                "CORS Response: This CORS request is not allowed")};
    }

    const auto& info = origin_info->get();

    auto response = http::response(http::status::no_content);
    response.set("Access-Control-Allow-Origin", std::move(origin));

    if (auto acrh = r.header("Access-Control-Request-Headers"); acrh) {
        auto rheaders = split<std::set<std::string>>(*acrh, ',');

        std::set<std::string> intersection;

        for (const auto& iheader : info.headers) {
            for (const auto& rheader : rheaders) {
                if (equals_wildcard_single(iheader, rheader)) {
                    intersection.insert(intersection.end(), rheader);
                }
            }
        }

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

    co_return result{.response = std::move(response)};
}

coro<result> module::flight(const http::request& request) const {
    auto rules = co_await get_info(request.bucket());

    auto origin = *request.header("origin");
    auto origin_info = find_info(*rules, origin, request.method());
    if (!origin_info) {
        co_return result{
            .response = error_response(
                http::status::forbidden, "Forbidden",
                "CORS Response: This CORS request is not allowed")};
    }

    const auto& info = origin_info->get();

    std::map<std::string, std::string> headers;
    headers["Access-Control-Allow-Origin"] = info.origin;
    if (info.expose_headers) {
        headers["Access-Control-Expose-Headers"] = *info.expose_headers;
    }

    co_return result{.headers = std::move(headers)};
}

coro<std::shared_ptr<std::vector<info>>>
module::get_info(const std::string& bucket) const {
    if (auto cached = m_info_cache.get(bucket); cached) {
        co_return *cached;
    }

    auto config = co_await m_directory.get_bucket_cors(bucket);
    if (!config) {
        throw command_exception(
            http::status::not_found, "NoSuchCORSConfiguration",
            "The specified bucket does not have a CORS configuration.");
    }

    auto parsed = std::make_shared<std::vector<info>>(parser::parse(*config));
    m_info_cache.put(bucket, parsed);
    co_return parsed;
}

} // namespace uh::cluster::ep::cors
