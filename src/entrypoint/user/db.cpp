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

coro<user> db::find_by_key(std::string key) {
    auto conn = co_await m_db.get();

    auto row = co_await conn->execv("SELECT id, username, secret_key, "
                                    "session_token, expires, arn, super_user "
                                    "FROM uh_query_key($1)",
                                    key);

    if (!row) {
        throw command_exception(status::not_found, "NoSuchAccessGrantError",
                                "The specified access grant does not exist.");
    }

    ep::user::key k{.id = std::move(key),
                    .secret_key = *row->string(2),
                    .session_token = row->string(3),
                    .expires = row->date(4)};

    user rv{.id = *row->string(0),
            .name = *row->string(1),
            .arn = row->string(5),
            .super_user = *row->boolean(6),
            .access_key = std::move(k)};

    for (auto row = co_await conn->execv(
             "SELECT name, value FROM uh_get_user_policy($1)", rv.name);
         row; row = co_await conn->next()) {

        auto name = row->string(0);
        auto json = row->string(1);

        if (!name || !json) {
            continue;
        }

        rv.policies[*name] = policy::parser::parse(*json);
        rv.policy_json[*name] = std::move(*json);
    }

    co_return rv;
}

coro<user> db::find(std::string id) {

    auto conn = co_await m_db.get();

    auto row = co_await conn->execv(
        "SELECT id, password, arn, super_user FROM uh_query_user($1)", id);

    if (!row) {
        throw std::runtime_error("unknown user id");
    }

    user rv{.id = *row->string(0),
            .name = id,
            .arn = row->string(2),
            .super_user = *row->boolean(3)};

    for (auto row = co_await conn->execv(
             "SELECT name, value FROM uh_get_user_policy($1)", rv.name);
         row; row = co_await conn->next()) {

        auto name = row->string(0);
        auto json = row->string(1);

        if (!name || !json) {
            continue;
        }

        rv.policies[*name] = policy::parser::parse(*json);
        rv.policy_json[*name] = std::move(*json);
    }

    co_return rv;
}

coro<user> db::find_and_check(std::string id, std::string pass) {

    auto conn = co_await m_db.get();

    auto row = co_await conn->execv(
        "SELECT id, password, arn, super_user FROM uh_query_user($1)", id);

    if (!row) {
        throw std::runtime_error("unknown user id");
    }

    if (!row->string(0)) {
        throw std::runtime_error("no password defined");
    }

    auto decoded = base64_decode(*row->string(1));
    auto fields = split(std::string_view(decoded.data(), decoded.size()), ':');

    auto pass_enc = m_crypt.derive(pass, std::string(fields[0]));
    if (pass_enc != fields[1]) {
        throw std::runtime_error("password mismatch");
    }

    user rv{.id = *row->string(0),
            .name = id,
            .arn = row->string(2),
            .super_user = *row->boolean(3)};

    for (auto row = co_await conn->execv(
             "SELECT name, value FROM uh_get_user_policy($1)", rv.name);
         row; row = co_await conn->next()) {

        auto name = row->string(0);
        auto json = row->string(1);

        if (!name || !json) {
            continue;
        }

        rv.policies[*name] = policy::parser::parse(*json);
        rv.policy_json[*name] = std::move(*json);
    }

    co_return rv;
}

coro<std::string> db::add_user(const std::string& name,
                               std::optional<std::string> password,
                               std::optional<std::string> arn) {

    auto conn = co_await m_db.get();

    std::optional<std::string> encoded;

    if (password) {
        std::string salt = random_string(48, SALT_CHARACTERS);
        auto pass_crypt = m_crypt.derive(*password, salt);
        auto pass_db = salt + ":" + pass_crypt;
        auto enc_vec = base64_encode(pass_db);
        encoded = std::string(&enc_vec[0], enc_vec.size());
    }

    try {
        auto row = co_await conn->execv("SELECT uh_add_user($1, $2, $3)", name,
                                        encoded, arn);

        if (!row->string(0)) {
            throw std::runtime_error("could not retrieve user id");
        }

        co_return *row->string(0);
    } catch (const std::exception& e) {
        LOG_DEBUG() << "error adding user: " << e.what();
        throw command_exception(status::conflict, "UserAlreadyExists",
                                "User already exists");
    }
}

coro<void> db::set_super_user(const std::string& name, bool flag) {
    auto conn = co_await m_db.get();
    co_await conn->execv("CALL uh_set_super_user($1, $2)", name, flag ? 1 : 0);
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

coro<void> db::remove_user(const std::string& username) {
    auto conn = co_await m_db.get();
    co_await conn->execv("CALL uh_remove_user($1)", username);
}

coro<void> db::policy(const std::string& user, const std::string& name,
                      const std::string& policy) {
    auto conn = co_await m_db.get();
    co_await conn->execv("CALL uh_put_user_policy($1, $2, $3)", user, name,
                         policy);
}

coro<std::string> db::policy(const std::string& user, const std::string& name) {
    auto conn = co_await m_db.get();
    auto row = co_await conn->execv(
        "SELECT value FROM uh_get_user_policy($1) WHERE name = $2", user, name);

    if (!row || !row->string(0)) {
        throw std::runtime_error("No policy found");
    }

    co_return *row->string(0);
}

coro<void> db::remove_policy(const std::string& user, const std::string& name) {
    auto conn = co_await m_db.get();
    co_await conn->execv("CALL uh_remove_user_policy($1, $2)", user, name);
}

coro<std::list<std::string>> db::list_user_policies(const std::string& user) {
    auto conn = co_await m_db.get();

    std::list<std::string> rv;

    for (auto row = co_await conn->execv(
             "SELECT name FROM uh_get_user_policy($1)", user);
         row; row = co_await conn->next()) {
        rv.emplace_back(*row->string(0));
    }

    co_return rv;
}

coro<std::list<key>> db::list_user_keys(const std::string& user) {
    std::list<key> rv;

    auto conn = co_await m_db.get();
    for (auto row = co_await conn->execv(
             "SELECT access_key, secret_key, session_token, expires FROM "
             "uh_list_user_keys($1)",
             user);
         row; row = co_await conn->next()) {

        rv.emplace_back(key{.id = *row->string(0),
                            .secret_key = *row->string(1),
                            .session_token = row->string(2),
                            .expires = row->date(3)});
    }

    co_return rv;
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
