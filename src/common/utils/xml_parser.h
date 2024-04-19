#pragma once

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <pugixml.hpp>
#include <span>
#include <string_view>

namespace uh::cluster {
namespace pt = boost::property_tree;

class xml_parser {
public:
    xml_parser() = default;

    bool parse(std::string_view body);

    std::vector<std::reference_wrapper<const pt::ptree>>
    get_nodes(pt::ptree::path_type&& path);

private:
    pt::ptree m_tree;

    template <typename Container>
    void enumerate(const pt::ptree& pt, pt::ptree::path_type path,
                   Container&& container) {
        if (path.empty())
            throw std::runtime_error("empty path given");

        if (path.single()) {
            const auto& name = path.reduce();
            for (const auto& child : pt) {
                if (child.first == name)
                    container.emplace_back(child.second);
            }
        } else {
            const auto& head = path.reduce();
            for (const auto& child : pt) {
                if (head == "*" || child.first == head) {
                    enumerate(child.second, path, container);
                }
            }
        }
    };
};
} // namespace uh::cluster
