#include "xml_parser.h"

#include <boost/iostreams/stream.hpp>

namespace uh::cluster {
bool xml_parser::parse(std::string_view body) {

    bool flag;
    try {
        boost::iostreams::stream<boost::iostreams::basic_array_source<char>>
            stream(body.begin(), body.size());

        pt::read_xml(stream, m_tree);
        flag = true;
    } catch (const std::exception& e) {
        flag = false;
    }

    return flag;
}

std::vector<std::reference_wrapper<const pt::ptree>>
xml_parser::get_nodes(pt::ptree::path_type&& path) {
    if (m_tree.empty()) [[unlikely]]
        return {};

    std::vector<std::reference_wrapper<const pt::ptree>> paths;
    enumerate(m_tree, path, paths);
    return paths;
}
} // namespace uh::cluster
