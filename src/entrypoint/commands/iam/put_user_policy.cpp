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
