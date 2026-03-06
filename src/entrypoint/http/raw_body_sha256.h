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

#include "raw_body.h"

#include <common/crypto/hash.h>

namespace uh::cluster::ep::http {

class raw_body_sha256 : public raw_body {
public:
    raw_body_sha256(stream& s, raw_request& req, std::string signature);

    coro<std::span<const char>> read(std::size_t count) override;

private:
    std::string m_signature;
    sha256 m_hash;
};

} // namespace uh::cluster::ep::http
