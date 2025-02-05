#include "parser.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

namespace uh::cluster::ep::cors {

namespace {

info parse_corse_info(const boost::property_tree::ptree& tree) {
    info rv;

    auto origins = tree.equal_range("AllowedOrigin");
    for (auto it = origins.first; it != origins.second; ++it) {
        rv.allowed_origins.insert(it->second.get_value<std::string>());
    }

    auto methods = tree.equal_range("AllowedMethod");
    for (auto it = methods.first; it != methods.second; ++it) {
        auto method = it->second.get_value<std::string>();
        if (method == "DELETE") {
            rv.allowed_methods.insert(http::verb::delete_);
        } else if (method == "GET") {
            rv.allowed_methods.insert(http::verb::get);
        } else if (method == "HEAD") {
            rv.allowed_methods.insert(http::verb::head);
        } else if (method == "POST") {
            rv.allowed_methods.insert(http::verb::post);
        } else if (method == "PUT") {
            rv.allowed_methods.insert(http::verb::put);
        }
    }

    return rv;
}

} // namespace

std::map<std::string, info> parser::parse(std::string code) {
    std::stringstream str(std::move(code));
    boost::property_tree::ptree tree;
    boost::property_tree::read_xml(str, tree);

    auto conf = tree.get_child_optional("CORSConfiguration");
    if (!conf) {
        return {};
    }

    std::map<std::string, info> rv;

    auto rules = conf->equal_range("CORSRule");
    for (auto it = rules.first; it != rules.second; ++it) {
        auto info = parse_corse_info(it->second);
        for (const auto& key : info.allowed_origins) {
            rv[key] = info;
        }
    }

    return rv;
}

} // namespace uh::cluster::ep::cors
