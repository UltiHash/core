#include "create_access_key.h"

#include <common/utils/random.h>

namespace uh::cluster::ep::iam {

create_access_key::create_access_key(user::db& users)
    : m_users(users) {}

coro<ep::http::response> create_access_key::handle(ep::http::request& req) {
    auto user = req.query("UserName");
    if (!user) {
        throw command_exception(ep::http::status::bad_request,
                                "InvalidArgument", "UserName missing.");
    }

    auto access_key = random_string(20, std::string(CHARS_CAPITALS) +
                                            std::string(CHARS_DIGITS));
    auto secret_key = random_string(
        32, std::string(CHARS_CAPITALS) + std::string(CHARS_LOWERCASE) +
                std::string(CHARS_DIGITS) + std::string("/"));

    co_await m_users.add_key(*user, access_key, secret_key, {}, {});

    boost::property_tree::ptree pt_key;
    pt_key.put("UserName", *user);
    pt_key.put("AccessKeyId", access_key);
    pt_key.put("Status", "Active");
    pt_key.put("SecretAccessKey", secret_key);

    boost::property_tree::ptree pt;
    pt.add_child("CreateAccessKeyResponse.CreateAccessKeyResult.AccessKey",
                 pt_key);

    http::response resp;
    resp << pt;
    co_return resp;
}

std::string create_access_key::action_id() const {
    return "iam::CreateAccessKey";
}

bool create_access_key::can_handle(const ep::http::request& req) {
    return req.method() == http::verb::post &&
           req.query("Action").value_or("") == "CreateAccessKey";
}

} // namespace uh::cluster::ep::iam
