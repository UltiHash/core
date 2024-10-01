#include "common/db/db.h"
#include "config/configuration.h"

#include "common/telemetry/log.h"
#include "common/utils/strings.h"
#include "entrypoint/formats.h"
#include "entrypoint/user/db.h"

#include <ranges>

using namespace uh::cluster;

struct config {
    enum class command {
        add_user,
        add_key,
        remove,
        policy,
        list,
        cleanup,
        info
    };

    uh::cluster::db::config database;

    command cmd = command::list;

    // add options
    struct {
        std::string username;
        std::string password;
        std::optional<std::string> arn;
    } add_user;

    // add options
    struct {
        std::string username;
        std::string access_id;
        std::string secret_key;
        std::optional<std::string> sts_token;
        std::optional<std::size_t> ttl;
    } add_key;

    struct {
        std::string access_id;
    } remove;

    struct {
        std::string username;
        std::optional<std::string> name;
        std::optional<std::string> policy;
        bool remove = false;
        bool get = false;
        bool put = false;
    } policy;

    struct {
        std::size_t expire_before = 0ull;
    } cleanup;

    struct {
        std::string username;
    } info;

    boost::log::trivial::severity_level log_level =
        boost::log::trivial::warning;
};

std::optional<::config> read_config(int argc, char** argv) {
    CLI::App app("UH user database control");
    argv = app.ensure_utf8(argv);

    ::config rv;

    uh::cluster::configure(app, rv.database);
    uh::cluster::configure(app, rv.log_level);

    auto* sub_add = app.add_subcommand("add", "add user to database");
    sub_add->add_option("username", rv.add_user.username, "user name");
    sub_add->add_option("password", rv.add_user.password, "password");
    sub_add->add_option("arn", rv.add_user.arn, "ARN");

    auto* sub_add_key =
        app.add_subcommand("add-key", "add access entry to database");
    sub_add_key->add_option("username", rv.add_key.username, "user name");
    sub_add_key->add_option("access-id", rv.add_key.access_id,
                            "entry's access id");
    sub_add_key->add_option("secret-key", rv.add_key.secret_key,
                            "entry's secret");
    sub_add_key->add_option("--sts-token", rv.add_key.sts_token,
                            "STS token string");
    sub_add_key->add_option("ttl", rv.add_key.ttl,
                            "number of seconds before expiration");

    auto* sub_remove =
        app.add_subcommand("remove", "remove access entry from database");
    sub_remove->add_option("access-id", rv.remove.access_id,
                           "entry's access id");

    auto* sub_policy = app.add_subcommand("policy", "modify user policies");
    sub_policy->add_option("username", rv.policy.username, "username");
    sub_policy->add_option("policy-name", rv.policy.name, "policy name");
    sub_policy->add_option("policy", rv.policy.policy, "policy JSON");
    sub_policy->add_option("--remove", rv.policy.remove, "delete the policy");
    sub_policy->add_option("--put", rv.policy.put, "put a policy");
    sub_policy->add_option("--get", rv.policy.get, "get the policy");

    auto sub_list = app.add_subcommand("list", "show stored access entries");

    auto sub_cleanup =
        app.add_subcommand("cleanup", "remove expired user accounts");
    sub_cleanup
        ->add_option(
            "--expire-before", rv.cleanup.expire_before,
            "only remove entries that expired before that many seconds")
        ->default_val(rv.cleanup.expire_before);

    auto sub_info =
        app.add_subcommand("info", "extensive information about a user");
    sub_info->add_option("username", rv.info.username, "username");

    try {
        app.parse(argc, argv);
    } catch (const CLI::Success& e) {
        app.exit(e);
        return {};
    }

    uh::log::set_level(rv.log_level);
    if (sub_add->parsed()) {
        rv.cmd = ::config::command::add_user;
    } else if (sub_add_key->parsed()) {
        rv.cmd = ::config::command::add_key;
    } else if (sub_remove->parsed()) {
        rv.cmd = ::config::command::remove;
    } else if (sub_policy->parsed()) {
        rv.cmd = ::config::command::policy;
    } else if (sub_list->parsed()) {
        rv.cmd = ::config::command::list;
    } else if (sub_cleanup->parsed()) {
        rv.cmd = ::config::command::cleanup;
    } else if (sub_info->parsed()) {
        rv.cmd = ::config::command::info;
    }

    return rv;
}

uh::cluster::coro<void> add_user(ep::user::db& db, const ::config& cfg) {
    std::string arn = cfg.add_user.arn
                          ? *cfg.add_user.arn
                          : "arn:uh:iam::0:" + cfg.add_user.username;

    co_await db.add_user(cfg.add_user.username, cfg.add_user.password, arn);
}

uh::cluster::coro<void> add_key(ep::user::db& db, const ::config& cfg) {
    co_await db.add_key(cfg.add_key.username, cfg.add_key.access_id,
                        cfg.add_key.secret_key, cfg.add_key.sts_token,
                        cfg.add_key.ttl);
}

uh::cluster::coro<void> remove_entry(ep::user::db& db, const ::config& cfg) {
    co_await db.remove_key(cfg.remove.access_id);
}

uh::cluster::coro<void> policy(ep::user::db& db, const ::config& cfg) {
    if (cfg.policy.remove) {
        if (!cfg.policy.name) {
            throw std::runtime_error("no policy name given");
        }

        co_await db.remove_policy(cfg.policy.username, *cfg.policy.name);
        co_return;
    }

    if (cfg.policy.get) {
        if (!cfg.policy.name) {
            throw std::runtime_error("no policy name given");
        }

        auto policy = co_await db.policy(cfg.policy.username, *cfg.policy.name);
        std::cout << policy << "\n";
        co_return;
    }

    if (cfg.policy.put) {
        if (!cfg.policy.policy) {
            throw std::runtime_error("no policy given");
        }

        if (!cfg.policy.name) {
            throw std::runtime_error("no policy name given");
        }

        co_await db.policy(cfg.policy.username, *cfg.policy.name,
                           *cfg.policy.policy);
        co_return;
    }

    auto policies = co_await db.list_user_policies(cfg.policy.username);
    for (const auto& pol_name : policies) {
        std::cout << pol_name << "\n";
    }
}

uh::cluster::coro<void> list_entries(ep::user::db& db, const ::config& cfg) {
    auto entries = co_await db.entries();

    for (const auto& entry : entries) {
        auto user = co_await db.find(entry);

        std::cout << user.id << "\t" << user.name << "\t"
                  << user.arn.value_or("<-- no ARN -->") << "\t"
                  << join(std::views::keys(user.policies), ", ") << "\n";
    }
}

uh::cluster::coro<void> cleanup(ep::user::db& db, const ::config& cfg) {
    co_await db.remove_expired(cfg.cleanup.expire_before);
}

uh::cluster::coro<void> info(ep::user::db& db, const ::config& cfg) {
    auto user = co_await db.find(cfg.info.username);

    std::cout << "id:\t" << user.id << "\n"
              << "name:\t" << user.name << "\n"
              << "arn:\t" << user.arn.value_or("<-- no ARN -->") << "\n\n";

    std::cout << "policies:\n";
    for (const auto& pol : user.policies) {
        std::cout << pol.first << "\n";
    }
    std::cout << "\n";

    std::cout << "keys:\n";
    auto keys = co_await db.list_user_keys(cfg.info.username);
    for (const auto& key : keys) {
        std::cout << key.id << "\t" << key.secret_key << "\t"
                  << key.session_token.value_or("<-- no STS token -->") << "\t";

        if (key.expires) {
            std::cout << iso8601_date(*key.expires);
        } else {
            std::cout << "<-- no expiration -->";
        }
        std::cout << "\n";
    }
}

int main(int argc, char** argv) {
    try {
        auto cfg = ::read_config(argc, argv);
        if (!cfg) {
            return 0;
        }

        boost::asio::io_context executor;
        auto handler = [](const std::exception_ptr& e) {
            if (e) {
                std::rethrow_exception(e);
            }
        };

        ep::user::db db(executor, cfg->database);

        switch (cfg->cmd) {
        case ::config::command::add_user:
            boost::asio::co_spawn(executor, add_user(db, *cfg), handler);
            break;
        case ::config::command::add_key:
            boost::asio::co_spawn(executor, add_key(db, *cfg), handler);
            break;
        case ::config::command::remove:
            boost::asio::co_spawn(executor, remove_entry(db, *cfg), handler);
            break;
        case ::config::command::policy:
            boost::asio::co_spawn(executor, policy(db, *cfg), handler);
            break;
        case ::config::command::list:
            boost::asio::co_spawn(executor, list_entries(db, *cfg), handler);
            break;
        case ::config::command::cleanup:
            boost::asio::co_spawn(executor, cleanup(db, *cfg), handler);
            break;
        case ::config::command::info:
            boost::asio::co_spawn(executor, info(db, *cfg), handler);
            break;
        default:
            throw std::runtime_error("unsupported command");
        }

        executor.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
