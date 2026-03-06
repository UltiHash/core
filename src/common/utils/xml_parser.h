/*
 * Copyright 2026 UltiHash Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <string_view>

namespace uh::cluster {
namespace pt = boost::property_tree;

class xml_parser {
public:
    xml_parser() = default;

    bool parse(std::string_view body);

    std::vector<std::reference_wrapper<const pt::ptree>>
    get_nodes(pt::ptree::path_type&& path);

private:
    pt::ptree m_tree;

    template <typename Container>
    void enumerate(const pt::ptree& pt, pt::ptree::path_type path,
                   Container&& container) {
        if (path.empty())
            throw std::runtime_error("empty path given");

        if (path.single()) {
            const auto& name = path.reduce();
            for (const auto& child : pt) {
                if (child.first == name)
                    container.emplace_back(child.second);
            }
        } else {
            const auto& head = path.reduce();
            for (const auto& child : pt) {
                if (head == "*" || child.first == head) {
                    enumerate(child.second, path, container);
                }
            }
        }
    };
};

} // namespace uh::cluster
