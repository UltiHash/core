#pragma once

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <pugixml.hpp>
#include <span>

namespace uh::cluster {
namespace pt = boost::property_tree;

class boost_xml_parser {
public:
    boost_xml_parser() = default;

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

namespace uh::cluster::rest::utils::parser {

//------------------------------------------------------------------------------

class xml_parser {
public:
    xml_parser() = default;
    ~xml_parser() = default;

    [[nodiscard]] bool parse(const std::string& xml_string);
    [[nodiscard]] pugi::xml_node get_root_element() const;
    [[nodiscard]] pugi::xpath_node_set
    get_nodes_from_path(const char* child_name) const;
    [[nodiscard]] std::string get_child_value(const pugi::xml_node& parent,
                                              const char* child_name) const;

private:
    pugi::xml_document m_doc;
};

//------------------------------------------------------------------------------

} // namespace uh::cluster::rest::utils::parser
