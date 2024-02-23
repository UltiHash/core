#pragma once

#include <pugixml.hpp>
#include <span>

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
