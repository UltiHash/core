#pragma once

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <pugixml.hpp>
#include <span>

namespace uh::cluster {
namespace pt = boost::property_tree;

class xml_parser {
public:
    xml_parser() = default;

    bool parse(const std::string& body);

    std::vector<std::reference_wrapper<const pt::ptree>>
    get_nodes(pt::ptree::path_type&& path);

private:
    pt::ptree m_tree;

    template <typename Tree, typename Out>
    Out enumerate(const Tree& pt, Tree::path_type path, Out out) {
        if (path.empty())
            return out;

        if (path.single()) {
            auto name = path.reduce();
            for (auto& child : pt) {
                if (child.first == name)
                    *out++ = child.second;
            }
        } else {
            auto head = path.reduce();
            for (auto& child : pt) {
                if (head == "*" || child.first == head) {
                    out = enumerate(child.second, path, out);
                }
            }
        }

        return out;
    };
};
} // namespace uh::cluster
