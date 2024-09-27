#include "db_backend.h"

#include "common/db/db.h"
#include "entrypoint/policy/parser.h"

namespace uh::cluster::ep::user {

db_backend::db_backend(ep::user::db db)
    : m_db(std::move(db)) {}

coro<user> db_backend::find(std::string_view access_id) {
    auto info = co_await m_db.find(access_id);

    co_return user{
        .secret_key = info.secret_key,
        .policies = policy::parser::parse(info.policy.value_or("")),
        // TODO compute ARN
    };
}

} // namespace uh::cluster::ep::user
