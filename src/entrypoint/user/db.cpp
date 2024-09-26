#include "db.h"

#include "entrypoint/policy/parser.h"

namespace uh::cluster::ep::user {

db::db(boost::asio::io_context& ioc, const uh::cluster::db::config& cfg)
    : m_db(ioc, connection_factory(ioc, cfg, cfg.users), cfg.users.count) {}

coro<user> db::find(std::string_view key) {
    auto conn = co_await m_db.get();

    auto row = co_await conn->execv(
        "SELECT username, secret_key, session_token, policy, "
        "expires FROM uh_query_key($1)",
        key);

    if (!row) {
        throw std::runtime_error("unknown access id: " + std::string(key));
    }

    co_return user{
        .name = *row->string(0),
        .secret_key = *row->string(1),
        .session_token = row->string(2),
        .policy_json = row->string(3),
        .policies = policy::parser::parse(row->string(2).value_or("")),
        .expires = row->date(4),
    };
}

coro<user> db::find(std::string_view id, std::string_view pass) {
    throw std::runtime_error("password based find not implemented");
}

coro<void> db::add_key(const std::string& username, const std::string& key,
                       const std::string& secret,
                       std::optional<std::string> sts,
                       std::optional<std::size_t> ttl) {
    auto conn = co_await m_db.get();
    if (ttl) {
        co_await conn->execv(
            "SELECT uh_add_user_key($1, $2, $3, $4, now()::timestamp + "
            "make_interval(secs => $5))",
            username, key, secret, sts, ttl);
    } else {
        co_await conn->execv("SELECT uh_add_user_key($1, $2, $3, $4, NULL)",
                             username, key, secret, sts);
    }
}

coro<void> db::remove_key(const std::string& key) {
    auto conn = co_await m_db.get();
    co_await conn->execv("CALL uh_remove_key($1)", key);
}

coro<void> db::policy(const std::string& name,
                      std::optional<std::string> policy) {
    auto conn = co_await m_db.get();
    co_await conn->execv("CALL uh_set_user_policy($1, $2)", name, policy);
}

coro<std::list<std::string>> db::entries() {
    std::list<std::string> rv;

    auto conn = co_await m_db.get();
    for (auto row = co_await conn->exec("SELECT username FROM uh_list_users()");
         row; row = co_await conn->next()) {
        rv.emplace_back(*row->string(0));
    }

    co_return rv;
}

coro<void> db::remove_expired(std::size_t seconds) {
    auto conn = co_await m_db.get();
    co_await conn->execv("CALL uh_remove_expired_keys(now()::timestamp - "
                         "make_interval(secs => $1))",
                         seconds);
}

} // namespace uh::cluster::ep::user
