#include "delete_user_policy.h"

namespace uh::cluster::ep::iam {

delete_user_policy::delete_user_policy(user::db& users)
    : m_users(users) {}

coro<ep::http::response> delete_user_policy::handle(ep::http::request& req) {
    auto username = req.query("UserName");
    if (!username) {
        throw command_exception(ep::http::status::bad_request,
                                "InvalidArgument", "UserName missing.");
    }

    auto policy_name = req.query("PolicyName");
    if (!policy_name) {
        throw command_exception(ep::http::status::bad_request,
                                "InvalidArgument", "PolicyName missing.");
    }

    co_await m_users.remove_policy(*username, *policy_name);

    boost::property_tree::ptree pt;
    pt.put("DeleteUserPolicyResponse", "");

    http::response resp;
    resp << pt;
    co_return resp;
}

std::string delete_user_policy::action_id() const {
    return "iam::DeleteUserPolicy";
}

bool delete_user_policy::can_handle(const ep::http::request& req) {
    return req.method() == http::verb::post &&
           req.query("Action").value_or("") == "DeleteUserPolicy";
}

} // namespace uh::cluster::ep::iam
