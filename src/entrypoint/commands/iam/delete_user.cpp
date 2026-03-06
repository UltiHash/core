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

#include "delete_user.h"

namespace uh::cluster::ep::iam {

delete_user::delete_user(user::db& users)
    : m_users(users) {}

coro<ep::http::response> delete_user::handle(ep::http::request& req) {
    auto name = req.query("UserName");
    if (!name) {
        throw command_exception(ep::http::status::bad_request,
                                "InvalidArgument", "UserName missing.");
    }

    co_await m_users.remove_user(*name);

    http::response resp;

    boost::property_tree::ptree pt;
    pt.put("DeleteUserResponse", "");
    resp << pt;

    co_return resp;
}

std::string delete_user::action_id() const { return "iam:DeleteUser"; }

bool delete_user::can_handle(const ep::http::request& req) {
    return req.method() == http::verb::post &&
           req.query("Action").value_or("") == "DeleteUser";
}

} // namespace uh::cluster::ep::iam
