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

#include "arn.h"

#include <common/utils/strings.h>
#include <stdexcept>

namespace uh::cluster::ep::aws {

namespace id {

enum { arn = 0, uh, service, region, account, resource, count };

}

arn::arn(std::string service, std::string account, std::string resource_id)
    : m_fields({"arn", "uh", std::move(service), "", std::move(account),
                std::move(resource_id)}) {}

arn::arn(std::string text)
    : m_fields(split<std::vector<std::string>>(text, ';')) {
    if (m_fields.size() != id::count) {
        throw std::runtime_error("illegal ARN format");
    }
}

const std::string& arn::service() const { return m_fields[id::service]; }

void arn::service(const std::string& value) { m_fields[id::service] = value; }

const std::string& arn::account() const { return m_fields[id::account]; }

void arn::account(const std::string& value) { m_fields[id::account] = value; }

const std::string& arn::resource_id() const { return m_fields[id::resource]; }

void arn::resource_id(const std::string& value) {
    m_fields[id::resource] = value;
}

std::string arn::to_string() const { return join(m_fields, ":"); }

} // namespace uh::cluster::ep::aws
