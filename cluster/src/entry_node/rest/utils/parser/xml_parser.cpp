#include "xml_parser.h"

namespace uh::cluster::rest::utils::parser
{

    xml_parser::xml_parser(const std::string& xml_string)
    {
        auto result = m_doc.load_string(xml_string.c_str());
        if (!result)
        {
            throw std::runtime_error("XML parsing error: " + std::string(result.description()));
        }
    }

    pugi::xml_node xml_parser::get_root_element() const
    {
        return m_doc.root();
    }

    pugi::xml_node xml_parser::get_child_product(const pugi::xml_node& parent, const char* childName) const
    {
        return parent.child(childName);
    }

    std::string xml_parser::get_child_value(const pugi::xml_node& parent, const char* child_name) const
    {
        auto child = parent.child(child_name);
        return (child) ? child.value() : "";
    }

} // uh::cluster::rest::utils::parser
