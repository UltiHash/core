#include "xml_parser.h"

namespace uh::cluster {
bool boost_xml_parser::parse(const std::string& body) {

    bool flag;
    try {
        pt::read_xml(body, m_tree);
        flag = true;
    } catch (const std::exception& e) {
        flag = false;
    }

    return flag;
}

std::vector<std::reference_wrapper<const pt::ptree>>
boost_xml_parser::get_nodes(pt::ptree::path_type&& path) {
    if (m_tree.empty())
        return {};

    std::vector<std::reference_wrapper<const pt::ptree>> paths;
    enumerate(m_tree, path, std::back_inserter(paths));
    return paths;
}
} // namespace uh::cluster

namespace uh::cluster::rest::utils::parser {

bool xml_parser::parse(const std::string& xml_string) {
    auto result = m_doc.load_string(xml_string.c_str());
    if (!result) {
        return false;
    } else {
        return true;
    }
}

pugi::xml_node xml_parser::get_root_element() const {
    return m_doc.first_child();
}

pugi::xpath_node_set
xml_parser::get_nodes_from_path(const char* childName) const {
    return m_doc.select_nodes(childName);
}

} // namespace uh::cluster::rest::utils::parser
