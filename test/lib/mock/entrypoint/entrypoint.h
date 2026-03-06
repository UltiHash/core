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

#include <entrypoint/commands/command.h>
#include <entrypoint/policy/variables.h>

namespace uh::cluster::test {

class mock_command : public command {
public:
    mock_command(const std::string& id = "");
    coro<ep::http::response> handle(ep::http::request&) override;
    coro<void> validate(const ep::http::request& req) override;
    std::string action_id() const override;

private:
    std::string m_id;
};

class mock_body : public ep::http::body {
public:
    coro<std::span<const char>> read(std::size_t len) override;
    std::optional<std::size_t> length() const override;
    coro<void> consume() override;
    std::size_t buffer_size() const override;
};

ep::http::request
make_request(const std::string& code,
             const std::string& principal = ep::user::user::ANONYMOUS_ARN);

ep::policy::variables
vars(std::initializer_list<std::pair<std::string, std::string>> v);

} // namespace uh::cluster::test
