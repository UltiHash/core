#include "put_user_policy.h"

#include <common/utils/random.h>

namespace uh::cluster::ep::iam {

put_user_policy::put_user_policy(user::db& users)
    : m_users(users) {}

coro<ep::http::response> put_user_policy::handle(ep::http::request& req) {
    if (!req.query("username")) {
        throw command_exception(ep::http::status::bad_request, "Invalid Input",
                                "username missing");
    }

    auto user = *req.query("username");
    auto access_key = random_string(20, std::string(CHARS_CAPITALS) +
                                            std::string(CHARS_DIGITS));
    auto secret_key = random_string(
        32, std::string(CHARS_CAPITALS) + std::string(CHARS_LOWERCASE) +
                std::string(CHARS_DIGITS) + std::string("/"));

    co_await m_users.add_key(user, access_key, secret_key, {}, {});

    boost::property_tree::ptree pt_key;
    pt_key.put("UserName", user);
    pt_key.put("AccessKey", access_key);
    pt_key.put("Status", "Active");
    pt_key.put("SecretAccessKey", secret_key);

    boost::property_tree::ptree pt;
    pt.add_child("CreateAccessKeyResponse.CreateAccessKeyResult.AccessKey",
                 pt_key);

    http::response resp;
    resp << pt;
    co_return resp;
}

std::string put_user_policy::action_id() const { return "iam::PutUserPolicy"; }

bool put_user_policy::can_handle(const ep::http::request& req) {
    return req.method() == http::verb::post &&
           req.query("action").value_or("") == "PutUserPolicy";
}

} // namespace uh::cluster::ep::iam
