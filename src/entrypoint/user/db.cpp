#include "db.h"

#include "entrypoint/http/command_exception.h"
#include "entrypoint/policy/parser.h"

#include <common/utils/random.h>
#include <common/utils/strings.h>

using namespace uh::cluster::ep::http;

namespace uh::cluster::ep::user {

namespace {

static const std::string SALT_CHARACTERS =
    "1234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "!@#$%^&*()_-+={}[];'|\\,.<>/?<>`~";
}

db::db(boost::asio::io_context& ioc, const uh::cluster::db::config& cfg)
    : m_db(ioc, connection_factory(ioc, cfg, cfg.users), cfg.users.count),
      m_crypt({}) {}

coro<user> db::find(std::string_view key) {
    auto conn = co_await m_db.get();

    auto row = co_await conn->execv(
        "SELECT username, secret_key, session_token, policy, "
        "expires, arn FROM uh_query_key($1)",
        key);

    if (!row) {
        throw command_exception(status::forbidden, "AccessDenied",
                                "Access Denied");
    }

    auto policy_json = row->string(3);
    std::list<ep::policy::policy> policies;
    if (policy_json) {
        policies = policy::parser::parse(*policy_json);
    }

    co_return user{.name = *row->string(0),
                   .secret_key = *row->string(1),
                   .session_token = row->string(2),
                   .policy_json = policy_json,
                   .policies = std::move(policies),
                   .expires = row->date(4),
                   .arn = row->string(5)};
}

coro<user> db::find(std::string id, std::string pass) {

    auto conn = co_await m_db.get();

    auto row = co_await conn->execv(
        "SELECT password, policy, arn FROM uh_query_user($1)", id);

    if (!row->string(0)) {
        throw std::runtime_error("no password defined");
    }

    auto decoded = base64_decode(*row->string(0));
    auto fields = split(std::string_view(decoded.data(), decoded.size()), ':');

    auto pass_enc = m_crypt.derive(pass, std::string(fields[0]));
    if (pass_enc != fields[1]) {
        throw std::runtime_error("password mismatch");
    }

    auto policy_json = row->string(1);
    std::list<ep::policy::policy> policies;
    if (policy_json) {
        policies = policy::parser::parse(*policy_json);
    }

    co_return user{.name = std::string(id),
                   .policy_json = policy_json,
                   .policies = policies,
                   .arn = row->string(2)};
}

coro<void> db::add_user(const std::string& name, const std::string& password,
                        std::optional<std::string> arn) {

    auto conn = co_await m_db.get();

    std::string salt = random_string(48, SALT_CHARACTERS);
    auto pass_crypt = m_crypt.derive(password, salt);
    auto pass_db = salt + ":" + pass_crypt;

    try {
        auto encoded = base64_encode(pass_db);
        co_await conn->execv("CALL uh_add_user($1, $2, $3)", name,
                             std::string_view(encoded.data(), encoded.size()),
                             arn);
    } catch (const std::exception& e) {
        LOG_DEBUG() << "error adding user: " << e.what();
        throw;
    }
}

coro<void> db::add_key(const std::string& username, const std::string& key,
                       const std::string& secret,
                       std::optional<std::string> sts,
                       std::optional<std::size_t> ttl) {
    auto conn = co_await m_db.get();
    if (ttl) {
        co_await conn->execv(
            "CALL uh_add_user_key($1, $2, $3, $4, now()::timestamp + "
            "make_interval(secs => $5))",
            username, key, secret, sts, ttl);
    } else {
        co_await conn->execv("CALL uh_add_user_key($1, $2, $3, $4, NULL)",
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
