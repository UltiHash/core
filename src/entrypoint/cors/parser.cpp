#include "parser.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <common/utils/strings.h>

namespace uh::cluster::ep::cors {

namespace {

std::pair<std::set<std::string>, info>
parse_corse_info(const boost::property_tree::ptree& tree) {
    std::set<std::string> rv_origins;

    auto origins = tree.equal_range("AllowedOrigin");
    for (auto it = origins.first; it != origins.second; ++it) {
        rv_origins.insert(it->second.get_value<std::string>());
    }

    info rv;
    auto methods = tree.equal_range("AllowedMethod");
    for (auto it = methods.first; it != methods.second; ++it) {
        auto method = it->second.get_value<std::string>();
        if (method == "DELETE") {
            rv.methods.insert(http::verb::delete_);
        } else if (method == "GET") {
            rv.methods.insert(http::verb::get);
        } else if (method == "HEAD") {
            rv.methods.insert(http::verb::head);
        } else if (method == "POST") {
            rv.methods.insert(http::verb::post);
        } else if (method == "PUT") {
            rv.methods.insert(http::verb::put);
        }
    }

    auto headers = tree.equal_range("AllowedHeader");
    for (auto it = headers.first; it != headers.second; ++it) {
        rv.headers.insert(it->second.get_value<std::string>());
    }

    auto expose = tree.equal_range("ExposeHeader");
    rv.expose_headers =
        join(std::ranges::subrange(expose.first, expose.second) |
                 std::views::transform([](auto& it) -> std::string {
                     return it.second.template get_value<std::string>();
                 }),
             ",");

    auto max_age_seconds = tree.get_optional<unsigned>("MaxAgeSeconds");
    if (max_age_seconds) {
        rv.max_age_seconds = std::move(max_age_seconds.value());
    }

    return std::make_pair(std::move(rv_origins), rv);
}

} // namespace

std::vector<info> parser::parse(std::string code) {
    std::stringstream str(std::move(code));
    boost::property_tree::ptree tree;
    boost::property_tree::read_xml(str, tree);

    auto conf = tree.get_child_optional("CORSConfiguration");
    if (!conf) {
        return {};
    }

    std::vector<info> rv;

    auto rules = conf->equal_range("CORSRule");
    for (auto it = rules.first; it != rules.second; ++it) {
        auto [origins, info] = parse_corse_info(it->second);
        for (const auto& key : origins) {
            info.origin = key;
            rv.push_back(info);
        }
    }

    return rv;
}

} // namespace uh::cluster::ep::cors
