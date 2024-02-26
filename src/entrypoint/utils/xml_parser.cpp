#include "xml_parser.h"

namespace uh::cluster {
bool xml_parser::parse(const std::string& body) {

    bool flag;
    try {
        std::istringstream string_stream(body);
        pt::read_xml(string_stream, m_tree);
        flag = true;
    } catch (const std::exception& e) {
        flag = false;
    }

    return flag;
}

std::vector<std::reference_wrapper<const pt::ptree>>
xml_parser::get_nodes(pt::ptree::path_type&& path) {
    if (m_tree.empty())
        return {};

    std::vector<std::reference_wrapper<const pt::ptree>> nodes_container;
    enumerate(m_tree, path, nodes_container);
    return nodes_container;
}
} // namespace uh::cluster
