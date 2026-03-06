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

#include <common/types/address.h>

#include <list>
#include <string>

namespace uh::cluster::ep::http {

struct range_spec {
    enum unit_t { bytes };

    struct range {
        // index of first byte
        std::size_t start = 0ull;
        // index directly after the last byte
        std::size_t end = 0ull;

        std::size_t length() const;
        std::string to_string() const;
    };

    std::list<range> ranges;
    unit_t unit;
};

/**
 * Parse an HTTP range.
 *
 * @param header value of the HTTP `Range` header
 * @param max maximum value in the range, used to compute negative ranges
 */
range_spec parse_range_header(std::string_view header, std::size_t max);

/**
 * Parse a single range specifier
 */
range_spec::range parse_range_spec(std::string_view header, std::size_t max);

/**
 * Apply the range specification to the given address.
 */
address apply_range(address addr, const range_spec& spec);

} // namespace uh::cluster::ep::http
