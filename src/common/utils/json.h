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

#include <nlohmann/json.hpp>

#include <functional>
#include <optional>
#include <set>
#include <string>

namespace uh::cluster {

using nlohmann::json;

inline const json& require(const json& j, std::string_view key) {
    auto it = j.find(key);
    if (it == j.end()) {
        throw std::runtime_error("required key `" + std::string(key) +
                                 "` not found");
    }

    return *it;
}

inline std::optional<std::reference_wrapper<const json>>
optional(const json& j, std::string_view key) {

    auto it = j.find(key);
    if (it == j.end()) {
        return {};
    }

    return *it;
}

inline std::string to_string(const json& element) {
    return element.get<std::string>();
}

template <template <typename...> typename container = std::set>
auto multi_element(const json& element,
                   std::invocable<const json&> auto reader) {

    using result = container<
        typename std::invoke_result<decltype(reader)&, const json&>::type>;

    if (!element.is_array()) {
        return result{reader(element)};
    }

    result rv;
    for (const auto& sub : element) {
        rv.insert(rv.end(), reader(sub));
    }
    return rv;
}

template <template <typename...> typename container = std::set>
auto multi_element(std::optional<std::reference_wrapper<const json>> element,
                   std::invocable<const json&> auto reader) {

    using result = container<
        typename std::invoke_result<decltype(reader)&, const json&>::type>;

    if (!element) {
        return result{};
    }

    return multi_element<container>(element->get(), reader);
}

} // namespace uh::cluster
