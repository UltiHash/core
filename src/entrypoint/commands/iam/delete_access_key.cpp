#include "delete_access_key.h"

namespace uh::cluster::ep::iam {

delete_access_key::delete_access_key(user::db& users)
    : m_users(users) {}

coro<ep::http::response> delete_access_key::handle(ep::http::request& req) {

    auto access_key = req.query("accesskeyid");
    if (!access_key) {
        throw command_exception(ep::http::status::bad_request, "Invalid Input",
                                "Access Key Id missing");
    }

    auto username = req.query("username");
    if (username) {
        auto user = co_await m_users.find_by_key(*access_key);
        if (user.name != *username) {
            throw command_exception(
                ep::http::status::conflict, "User Name Mismatch",
                "The provided access key does not belong to the given user.");
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
           req.query("action").value_or("") == "DeleteAccessKey";
}

} // namespace uh::cluster::ep::iam
