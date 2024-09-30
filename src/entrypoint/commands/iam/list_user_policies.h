#ifndef CORE_ENTRYPOINT_COMMANDS_IAM_LIST_USER_POLICIES_H
#define CORE_ENTRYPOINT_COMMANDS_IAM_LIST_USER_POLICIES_H

#include <entrypoint/commands/command.h>
#include <entrypoint/user/db.h>

namespace uh::cluster::ep::iam {

class list_user_policies : public command {
public:
    list_user_policies(user::db& users);

    coro<ep::http::response> handle(ep::http::request&) override;
    std::string action_id() const override;

    static bool can_handle(const ep::http::request& req);

private:
    user::db& m_users;
};

} // namespace uh::cluster::ep::iam

#endif
