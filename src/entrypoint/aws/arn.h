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

#include <string>
#include <vector>

namespace uh::cluster::ep::aws {

class arn {
public:
    arn(std::string service, std::string account, std::string resource_id);
    arn(std::string text);

    const std::string& service() const;
    void service(const std::string& value);

    const std::string& account() const;
    void account(const std::string& value);

    const std::string& resource_id() const;
    void resource_id(const std::string& value);

    std::string to_string() const;

private:
    std::vector<std::string> m_fields;
};

} // namespace uh::cluster::ep::aws
