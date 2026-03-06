// Copyright 2026 UltiHash Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

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
