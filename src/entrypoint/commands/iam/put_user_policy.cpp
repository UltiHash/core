#include "put_user_policy.h"

#include <common/utils/random.h>

namespace uh::cluster::ep::iam {

put_user_policy::put_user_policy(user::db& users)
    : m_users(users) {}

coro<ep::http::response> put_user_policy::handle(ep::http::request& req) {
    auto user = req.query("UserName");
    if (!user) {
        throw command_exception(ep::http::status::bad_request, "Invalid Input",
                                "username missing");
    }

    (void)m_users;
    http::response resp;
    co_return resp;
}

std::string put_user_policy::action_id() const { return "iam::PutUserPolicy"; }

bool put_user_policy::can_handle(const ep::http::request& req) {
    return req.method() == http::verb::post &&
           req.query("Action").value_or("") == "PutUserPolicy";
}

} // namespace uh::cluster::ep::iam
