#ifndef CORE_ENTRYPOINT_COMMANDS_IAM_PUT_USER_POLICY_H
#define CORE_ENTRYPOINT_COMMANDS_IAM_PUT_USER_POLICY_H

#include "entrypoint/http/user_db.h"
#include <entrypoint/commands/command.h>

namespace uh::cluster::ep::iam {

class put_user_policy : public command {
public:
    put_user_policy(user::db& users);

    coro<ep::http::response> handle(ep::http::request&) override;
    std::string action_id() const override;

    static bool can_handle(const ep::http::request& req);

private:
    user::db& m_users;
};

} // namespace uh::cluster::ep::iam

#endif
