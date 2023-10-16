#pragma once

#include <pugixml.hpp>
#include <span>

namespace uh::cluster::rest::utils::parser
{

//------------------------------------------------------------------------------

    class xml_parser
    {
    public:
        explicit xml_parser(const std::string& xml_string);
        ~xml_parser() = default;

        [[nodiscard]] pugi::xml_node get_root_element() const;
        [[nodiscard]] pugi::xml_node get_child_product(const pugi::xml_node& parent, const char* child_name) const;
        [[nodiscard]] std::string get_child_value(const pugi::xml_node& parent, const char* child_name) const;

    private:
        pugi::xml_document m_doc;
    };

//------------------------------------------------------------------------------

} // uh::cluster::rest::utils::parser
