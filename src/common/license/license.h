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

#include <magic_enum/magic_enum.hpp>
#include <string_view>
#include <vector>

namespace uh::cluster {

struct license {
    enum type { NONE, FREEMIUM, PREMIUM };

    std::string version;
    std::string customer_id;
    enum type license_type { NONE };
    std::size_t storage_cap_gib{0};

    operator bool() const { return is_valid(); }

    enum class verify : std::uint8_t { VERIFY, SKIP_VERIFY };

    static license create(const std::string& json_str,
                          verify option = verify::VERIFY);

    std::string to_string() const { return m_compact_json; };

    std::vector<std::pair<std::string, std::string>>
    to_key_value_iterable() const;

private:
    bool is_valid() const { return license_type != type::NONE; }

    std::string m_compact_json;
};

} // namespace uh::cluster
