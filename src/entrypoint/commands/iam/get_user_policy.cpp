#include "get_user_policy.h"

namespace uh::cluster::ep::iam {

get_user_policy::get_user_policy(user::db& users)
    : m_users(users) {}

coro<ep::http::response> get_user_policy::handle(ep::http::request& req) {
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

    auto policy = co_await m_users.policy(*username, *policy_name);

    boost::property_tree::ptree pt;
    pt.put("GetUserPolicyResponse.GetUserPolicyResult.UserName", *username);
    pt.put("GetUserPolicyResponse.GetUserPolicyResult.PolicyName",
           *policy_name);
    pt.put("GetUserPolicyResponse.GetUserPolicyResult.PolicyDocument", policy);

    http::response resp;
    resp << pt;
    co_return resp;
}

std::string get_user_policy::action_id() const { return "iam::GetUserPolicy"; }

bool get_user_policy::can_handle(const ep::http::request& req) {
    return req.method() == http::verb::post &&
           req.query("Action").value_or("") == "GetUserPolicy";
}

} // namespace uh::cluster::ep::iam
