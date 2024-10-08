#ifndef CORE_ENTRYPOINT_COMMANDS_IAM_GET_USER_POLICY_H
#define CORE_ENTRYPOINT_COMMANDS_IAM_GET_USER_POLICY_H

#include <entrypoint/commands/command.h>
#include <entrypoint/user/db.h>

namespace uh::cluster::ep::iam {

class get_user_policy : public command {
public:
    get_user_policy(user::db& users);

    coro<ep::http::response> handle(ep::http::request&) override;
    std::string action_id() const override;

    static bool can_handle(const ep::http::request& req);

private:
    user::db& m_users;
};

} // namespace uh::cluster::ep::iam

#endif
