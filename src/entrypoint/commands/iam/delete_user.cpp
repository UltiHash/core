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
