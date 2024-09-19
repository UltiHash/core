#include "db_backend.h"

#include "common/db/db.h"

namespace uh::cluster::ep::user {

db_backend::db_backend(boost::asio::io_context& ioc, const db::config& cfg)
    : m_db(ioc, connection_factory(ioc, cfg, cfg.users), cfg.users.count) {}

coro<user> db_backend::find(std::string_view access_id) {
    auto conn = co_await m_db.get();

    auto row = co_await conn->execv(
        "SELECT secret_key, policy FROM uh_query_user($1, $2)", access_id,
        std::optional<std::string>());
    if (!row) {
        throw std::runtime_error("unknown access id");
    }

    co_return user{
        .secret_key = *row->string(0),
        .policies = policy::parser::parse(row->string(1).value_or(""))
        // TODO compute ARN
    };
}

} // namespace uh::cluster::ep::user
