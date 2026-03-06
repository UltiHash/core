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

#include "host_utils.h"

namespace uh::cluster {

bool is_valid_ip(const std::string& ip) {
    try {
        auto address = boost::asio::ip::make_address(ip);
        return address.is_v4() || address.is_v6();
    } catch (const std::exception& e) {
        return false;
    }
}

std::string get_host() {
    const char* var_value = std::getenv(ENV_CFG_ENDPOINT_HOST);
    if (var_value == nullptr) {
        return boost::asio::ip::host_name();
    } else {
        if (is_valid_ip(var_value))
            return {var_value};
        else
            throw std::invalid_argument(
                "the environmental variable " +
                std::string(ENV_CFG_ENDPOINT_HOST) +
                " does not contain a valid IPv4 or IPv6 address: '" +
                std::string(var_value) + "'");
    }
}
} // namespace uh::cluster
