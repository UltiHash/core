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
        return m_doc.first_child();
    }

    pugi::xpath_node_set xml_parser::get_nodes_from_path(const char* childName) const
    {
        return m_doc.select_nodes(childName);
    }

} // uh::cluster::rest::utils::parser
