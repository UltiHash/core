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

#include <memory>
#include <openssl/kdf.h>
#include <string>

namespace uh::cluster {

class scrypt {
public:
    struct config {
        uint64_t n = 1u << 17;
        uint32_t r = 8u;
        uint32_t p = 1u;

        uint32_t length = 32u;
    };

    scrypt(const config& c);

    std::string derive(std::string password, std::string salt);

private:
    config m_c;
};

} // namespace uh::cluster
