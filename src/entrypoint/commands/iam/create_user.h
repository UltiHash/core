#ifndef CORE_ENTRYPOINT_COMMANDS_IAM_CREATE_USER_H
#define CORE_ENTRYPOINT_COMMANDS_IAM_CREATE_USER_H

#include "entrypoint/http/user_db.h"
#include <entrypoint/commands/command.h>

namespace uh::cluster::ep::iam {

class create_user : public command {
public:
    create_user(user::db& users);

    coro<ep::http::response> handle(ep::http::request&) override;
    std::string action_id() const override;

    static bool can_handle(const ep::http::request& req);

private:
    user::db& m_users;
};

} // namespace uh::cluster::ep::iam

#endif
