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

namespace uh::cluster::ep::http {

class string_body : public body {
public:
    string_body(std::string&& body);

    std::optional<std::size_t> length() const override;
    coro<std::span<const char>> read(std::size_t count) override;
    coro<void> consume() override;

    const std::string& get_body() const { return m_body; }

    std::size_t buffer_size() const override;
private:
    std::string m_body;
    std::size_t m_read;
};

} // namespace uh::cluster::ep::http
