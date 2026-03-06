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

#include "common/types/common_types.h"
#include "entrypoint/policy/policy.h"
#include <list>
#include <optional>
#include <string>

namespace uh::cluster::ep::user {

struct key {
    std::string id;
    std::string secret_key;
    std::optional<std::string> session_token;
    std::optional<utc_time> expires;
};

struct user {
    std::string id;
    std::string name;
    std::optional<std::string> arn = ANONYMOUS_ARN;
    bool super_user = false;

    std::map<std::string, std::string> policy_json;
    std::map<std::string, std::list<policy::policy>> policies;

    std::optional<key> access_key;

    inline static const std::string ANONYMOUS = "anonymous";
    inline static const std::string ANONYMOUS_ARN = "arn:aws:iam::1:anonymous";
};

} // namespace uh::cluster::ep::user
