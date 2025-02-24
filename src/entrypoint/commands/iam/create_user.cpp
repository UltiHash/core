#include "create_user.h"

#include <entrypoint/aws/arn.h>
#include <entrypoint/http/response.h>

namespace uh::cluster::ep::iam {

create_user::create_user(user::db& users)
    : m_users(users) {}

coro<ep::http::response> create_user::handle(ep::http::request& req) {
    auto name = req.query("UserName");
    if (!name) {
        throw command_exception(ep::http::status::bad_request,
                                "ValidationError", "UserName missing.");
    }

    auto path = req.query("path").value_or("/");

    std::string user_path = std::string("user") + path + *name;
    aws::arn arn("iam", erase_all(req.authenticated_user().id, "-"), user_path);

    auto id = co_await m_users.add_user(*name, {}, arn.to_string());

    http::response resp;

    boost::property_tree::ptree pt;
    pt.put("CreateUserResponse.CreateUserResult.User.Path", path);
    pt.put("CreateUserResponse.CreateUserResult.User.UserName", *name);
    pt.put("CreateUserResponse.CreateUserResult.User.UserId", id);
    pt.put("CreateUserResponse.CreateUserResult.User.Arn", arn.to_string());
    resp << pt;

    co_return resp;
}

std::string create_user::action_id() const { return "iam:CreateUser"; }

bool create_user::can_handle(const ep::http::request& req) {
    return req.method() == http::verb::post &&
           req.query("Action").value_or("") == "CreateUser";
}

} // namespace uh::cluster::ep::iam
