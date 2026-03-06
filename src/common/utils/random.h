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

#include "common/types/scoped_buffer.h"
#include <random>
#include <string>

namespace uh::cluster {

// ---------------------------------------------------------------------

std::string random_string(
    std::size_t length = 16,
    const std::string& chars = "0123456789abcdefghijklmnopqrstuvwxyz");

// ---------------------------------------------------------------------

template <std::integral value_type> class random_generator {
public:
    random_generator(value_type min = {},
                     value_type max = std::numeric_limits<value_type>::max())
        : m_rg(std::random_device{}()),
          m_pick(min, max) {}

    value_type operator()() { return m_pick(m_rg); }

private:
    std::mt19937 m_rg;
    std::uniform_int_distribution<value_type> m_pick;
};

std::string generate_unique_id();

// ---------------------------------------------------------------------

} // namespace uh::cluster
