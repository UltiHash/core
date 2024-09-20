#include "db.h"

namespace uh::cluster::ep::user {

db::db(boost::asio::io_context& ioc, const uh::cluster::db::config& cfg)
    : m_db(ioc, connection_factory(ioc, cfg, cfg.users), cfg.users.count) {}

coro<db::user_info> db::find(std::string_view key) {
    auto conn = co_await m_db.get();

    auto row = co_await conn->execv("SELECT secret_key, session_token, policy, "
                                    "expires FROM uh_query_user($1)",
                                    key);

    if (!row) {
        throw std::runtime_error("unknown access id: " + std::string(key));
    }

    co_return user_info{
        .secret_key = *row->string(0),
        .session_token = row->string(1),
        .policy = row->string(2),
        .expires = row->date(3),
    };
}

coro<void> db::add(const std::string& key, const std::string& secret,
                   std::optional<std::string> sts,
                   std::optional<std::size_t> ttl) {
    auto conn = co_await m_db.get();
    if (ttl) {
        co_await conn->execv(
            "SELECT uh_create_user($1, $2, $3, now()::timestamp + "
            "make_interval(secs => $4))",
            key, secret, sts, ttl);
    } else {
        co_await conn->execv("SELECT uh_create_user($1, $2, $3, NULL)", key,
                             secret, sts);
    }
}

coro<void> db::remove(const std::string& key) {
    auto conn = co_await m_db.get();
    co_await conn->execv("CALL uh_delete_user(key)", key);
}

coro<void> db::policy(const std::string& key,
                      std::optional<std::string> policy) {
    auto conn = co_await m_db.get();
    co_await conn->execv("CALL uh_set_user_policy($1, $2)", key, policy);
}

coro<std::list<std::string>> db::entries() {
    std::list<std::string> rv;

    auto conn = co_await m_db.get();
    for (auto row =
             co_await conn->exec("SELECT key FROM uh_list_access_keys()");
         row; row = co_await conn->next()) {
        rv.emplace_back(*row->string(0));
    }

    co_return rv;
}

coro<void> db::remove_expired() {
    auto conn = co_await m_db.get();
    co_await conn->execv("CALL uh_remove_expired_users()");
}

} // namespace uh::cluster::ep::user
