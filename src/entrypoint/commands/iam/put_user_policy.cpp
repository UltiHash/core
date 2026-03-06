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

#include "put_user_policy.h"

#include <common/utils/random.h>
#include <entrypoint/policy/parser.h>

namespace uh::cluster::ep::iam {

put_user_policy::put_user_policy(user::db& users)
    : m_users(users) {}

coro<ep::http::response> put_user_policy::handle(ep::http::request& req) {
    auto user = req.query("UserName");
    if (!user) {
        throw command_exception(ep::http::status::bad_request,
                                "InvalidArgument", "UserName missing.");
    }

    auto name = req.query("PolicyName");
    if (!name) {
        throw command_exception(ep::http::status::bad_request,
                                "InvalidArgument", "PolicyName missing.");
    }

    auto document = req.query("PolicyDocument");
    if (!document) {
        throw command_exception(ep::http::status::bad_request,
                                "InvalidArgument", "PolicyDocument missing.");
    }

    policy::parser::parse(*document);

    co_await m_users.policy(*user, *name, *document);

    boost::property_tree::ptree pt;
    pt.put("PutUserPolicyResponse", "");

    http::response resp;
    resp << pt;
    co_return resp;
}

std::string put_user_policy::action_id() const { return "iam::PutUserPolicy"; }

bool put_user_policy::can_handle(const ep::http::request& req) {
    return req.method() == http::verb::post &&
           req.query("Action").value_or("") == "PutUserPolicy";
}

} // namespace uh::cluster::ep::iam
