#include "parser.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

namespace uh::cluster::ep::cors {

namespace {

cors_info parse_corse_info(const boost::property_tree::ptree& tree) {
    cors_info info;

    info.allowed_origin = tree.get<std::string>("AllowedOrigin");

    auto methods = tree.equal_range("AllowedMethod");
    for (auto it = methods.first; it != methods.second; ++it) {
        auto method = it->second.get_value<std::string>();
        if (method == "DELETE") {
            info.allowed_delete = true;
        } else if (method == "GET") {
            info.allowed_get = true;
        } else if (method == "HEAD") {
            info.allowed_head = true;
        } else if (method == "POST") {
            info.allowed_post = true;
        } else if (method == "PUT") {
            info.allowed_put = true;
        }
    }

    return info;
}

} // namespace

std::map<std::string, cors_info> parser::parse(std::string code) {
    std::stringstream str(std::move(code));
    boost::property_tree::ptree tree;
    boost::property_tree::read_xml(str, tree);

    auto conf = tree.get_child_optional("CORSConfiguration");
    if (!conf) {
        return {};
    }

    std::map<std::string, cors_info> rv;

    auto rules = conf->equal_range("CORSRule");
    for (auto it = rules.first; it != rules.second; ++it) {
        auto info = parse_corse_info(it->second);
        rv[info.allowed_origin] = std::move(info);
    }

    return rv;
}

} // namespace uh::cluster::ep::cors
