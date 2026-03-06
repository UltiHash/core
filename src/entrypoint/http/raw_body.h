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

#include "body.h"
#include "raw_request.h"
#include "stream.h"

namespace uh::cluster::ep::http {

class raw_body : public body {
public:
    raw_body(stream& s, raw_request& req);

    std::optional<std::size_t> length() const override;

    coro<std::span<const char>> read(std::size_t count) override;
    coro<void> consume() override;

    std::size_t buffer_size() const override;

private:
    stream& m_s;
    std::size_t m_length;
};

} // namespace uh::cluster::ep::http
