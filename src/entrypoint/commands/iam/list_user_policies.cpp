#include "list_user_policies.h"

namespace uh::cluster::ep::iam {

list_user_policies::list_user_policies(user::db& users)
    : m_users(users) {}

coro<ep::http::response> list_user_policies::handle(ep::http::request& req) {
    auto username = req.query("UserName");
    if (!username) {
        throw command_exception(ep::http::status::bad_request, "Invalid Input",
                                "UserName missing.");
    }

    boost::property_tree::ptree pt_names;
    auto policies = co_await m_users.list_user_policies(*username);
    for (const auto& policy : policies) {
        pt_names.add("member", policy);
    }

    boost::property_tree::ptree pt;
    pt.add_child("ListUserPoliciesResponse.ListUserPoliciesResult.PolicyNames",
                 pt_names);
    pt.put("ListUserPoliciesResponse.ListUserPoliciesResult.IsTruncated",
           false);

    http::response resp;
    resp << pt;
    co_return resp;
}

std::string list_user_policies::action_id() const {
    return "iam::ListUserPolicies";
}

bool list_user_policies::can_handle(const ep::http::request& req) {
    return req.method() == http::verb::post &&
           req.query("Action").value_or("") == "ListUserPolicies";
}

} // namespace uh::cluster::ep::iam
