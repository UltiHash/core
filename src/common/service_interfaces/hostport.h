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

#include <cstdint>
#include <stdexcept>
#include <string>

namespace uh::cluster {

struct hostport {
    std::string hostname{};
    uint16_t port{0};
    std::string to_string() const {
        return hostname + ":" + std::to_string(port);
    }
    static hostport create(const std::string& str) {
        size_t pos = str.rfind(':');
        if (pos == std::string_view::npos) {
            throw std::invalid_argument("Invalid hostport format, missing ':'");
        }

        std::string hostname(str.substr(0, pos));
        std::string_view port_str = std::string_view(str).substr(pos + 1);

        uint16_t port;
        try {
            unsigned long port_ul = std::stoul(std::string(port_str));
            if (port_ul > UINT16_MAX) {
                throw std::out_of_range("Port number exceeds uint16_t range");
            }
            port = static_cast<uint16_t>(port_ul);
        } catch (const std::exception& e) {
            throw std::invalid_argument("Invalid port number");
        }

        return {hostname, port};
    }
};

} // namespace uh::cluster
