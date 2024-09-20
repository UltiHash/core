#include "common/db/db.h"
#include "config/configuration.h"

#include "common/telemetry/log.h"
#include "entrypoint/formats.h"
#include "entrypoint/user/db.h"

using namespace uh::cluster;

struct config {
    enum class command { add, remove, policy, list, cleanup };

    uh::cluster::db::config database;

    command cmd = command::list;

    // add options
    struct {
        std::string access_id;
        std::string secret_key;
        std::optional<std::string> sts_token;
        std::optional<std::size_t> ttl;
    } add;

    struct {
        std::string access_id;
    } remove;

    struct {
        std::string access_id;
        std::optional<std::string> policy;
        bool remove = false;
    } policy;

    boost::log::trivial::severity_level log_level =
        boost::log::trivial::warning;
};

std::optional<::config> read_config(int argc, char** argv) {
    CLI::App app("UH user database control");
    argv = app.ensure_utf8(argv);

    ::config rv;

    uh::cluster::configure(app, rv.database);
    uh::cluster::configure(app, rv.log_level);

    auto* sub_add = app.add_subcommand("add", "add access entry to database");
    sub_add->add_option("access-id", rv.add.access_id, "entry's access id");
    sub_add->add_option("secret-key", rv.add.secret_key, "entry's secret");
    sub_add->add_option("--sts-token", rv.add.sts_token, "STS token string");
    sub_add->add_option("ttl", rv.add.ttl,
                        "number of seconds before expiration");

    auto* sub_remove =
        app.add_subcommand("remove", "remove access entry from database");
    sub_remove->add_option("access-id", rv.remove.access_id,
                           "entry's access id");

    auto* sub_policy =
        app.add_subcommand("policy", "set or read entry's policy");
    sub_policy->add_option("access-id", rv.policy.access_id,
                           "entry's access id");
    sub_policy->add_option("policy", rv.policy.policy,
                           "set the policy to this");
    sub_policy->add_option("--remove", rv.policy.remove, "delete the policy");

    auto sub_list = app.add_subcommand("list", "show stored access entries");
    auto sub_cleanup =
        app.add_subcommand("cleanup", "remove expired user accounts");

    try {
        app.parse(argc, argv);
    } catch (const CLI::Success& e) {
        app.exit(e);
        return {};
    }

    uh::log::set_level(rv.log_level);
    if (sub_add->parsed()) {
        rv.cmd = ::config::command::add;
    } else if (sub_remove->parsed()) {
        rv.cmd = ::config::command::remove;
    } else if (sub_policy->parsed()) {
        rv.cmd = ::config::command::policy;
    } else if (sub_list->parsed()) {
        rv.cmd = ::config::command::list;
    } else if (sub_cleanup->parsed()) {
        rv.cmd = ::config::command::cleanup;
    }

    return rv;
}

uh::cluster::coro<void> add_entry(ep::user::db& db, const ::config& cfg) {
    co_await db.add(cfg.add.access_id, cfg.add.secret_key, cfg.add.sts_token,
                    cfg.add.ttl);
}

uh::cluster::coro<void> remove_entry(ep::user::db& db, const ::config& cfg) {
    co_await db.remove(cfg.remove.access_id);
}

uh::cluster::coro<void> policy(ep::user::db& db, const ::config& cfg) {
    if (cfg.policy.remove) {
        co_await db.policy(cfg.policy.access_id, std::nullopt);
        co_return;
    }

    if (cfg.policy.policy) {
        co_await db.policy(cfg.policy.access_id, cfg.policy.policy);
    } else {
        auto user_info = co_await db.find(cfg.policy.access_id);
        std::cout << user_info.policy.value_or("-- no policy --") << "\n";
    }
}

uh::cluster::coro<void> list_entries(ep::user::db& db, const ::config& cfg) {
    auto entries = co_await db.entries();

    for (const auto& entry : entries) {
        auto info = co_await db.find(entry);

        std::string expires = " -- no expiration -- ";
        if (info.expires) {
            expires = iso8601_date(*info.expires);
        }

        std::cout << entry << "\t"
                  << info.session_token.value_or("-- no session token --")
                  << "\t" << (info.policy ? " policy set " : " no policy ")
                  << "\t" << expires << "\n";
    }
}

uh::cluster::coro<void> cleanup(ep::user::db& db, const ::config&) {
    co_await db.remove_expired();
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
        case ::config::command::add:
            boost::asio::co_spawn(executor, add_entry(db, *cfg), handler);
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
        default:
            throw std::runtime_error("unsupported command");
        }

        executor.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
