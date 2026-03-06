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

#include "delete_access_key.h"

namespace uh::cluster::ep::iam {

delete_access_key::delete_access_key(user::db& users)
    : m_users(users) {}

coro<ep::http::response> delete_access_key::handle(ep::http::request& req) {

    auto access_key = req.query("AccessKeyId");
    if (!access_key) {
        throw command_exception(ep::http::status::bad_request,
                                "InvalidArgument", "Access Key Id missing.");
    }

    auto username = req.query("UserName");
    if (username) {
        auto user = co_await m_users.find_by_key(*access_key);
        if (user.name != *username) {
            throw command_exception(
                ep::http::status::conflict, "UserNameMismatch",
                "AWS IAM implements sophisticated organizations/roles "
                "management that allows administrator users to delete key of "
                "other users. We do not implement something like this and "
                "allow user only to delete their own key.");
        }
    }

    co_await m_users.remove_key(*access_key);

    boost::property_tree::ptree pt;
    pt.put("DeleteAccessKeyResponse", "");

    http::response resp;
    resp << pt;
    co_return resp;
}

std::string delete_access_key::action_id() const {
    return "iam::DeleteAccessKey";
}

bool delete_access_key::can_handle(const ep::http::request& req) {
    return req.method() == http::verb::post &&
           req.query("Action").value_or("") == "DeleteAccessKey";
}

} // namespace uh::cluster::ep::iam
