#pragma once

#include <entrypoint/commands/command.h>
#include <entrypoint/user/db.h>

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
