#include "common/db/db.h"
#include "config/configuration.h"
#include "entrypoint/directory.h"
#include "entrypoint/formats.h"

#include <CLI/CLI.hpp>
#include <iostream>

using uh::cluster::directory;
using uh::cluster::imf_fixdate;
using uh::cluster::object;

struct config {
    enum class command { ls, mkb, rmb };

    uh::cluster::db::config database;

    command cmd = command::ls;
    std::string target_ls;
    std::string target_mkb;
    std::string target_rmb;
};

std::optional<config> read_config(int argc, char** argv) {
    CLI::App app("UH directory CLI");
    argv = app.ensure_utf8(argv);

    config rv;

    uh::cluster::configure(app, rv.database);

    auto* sub_ls = app.add_subcommand("ls", "list contents of directory");
    sub_ls->add_option("bucket", rv.target_ls, "list contents of this bucket");

    auto* sub_mkb = app.add_subcommand("mkb", "make bucket");
    sub_mkb->add_option("bucket", rv.target_mkb, "name of bucket to create");

    auto* sub_rmb = app.add_subcommand("rmb", "remove bucket");
    sub_rmb->add_option("bucket", rv.target_rmb, "name of bucket to remove");

    try {
        app.parse(argc, argv);
    } catch (const CLI::Success& e) {
        app.exit(e);
        return {};
    }

    if (sub_ls->parsed()) {
        rv.cmd = config::command::ls;
    } else if (sub_mkb->parsed()) {
        rv.cmd = config::command::mkb;
    } else if (sub_rmb->parsed()) {
        rv.cmd = config::command::rmb;
    }

    return rv;
}

std::ostream& operator<<(std::ostream& out, const object& obj) {
    out << obj.name << "\t" << obj.size << "\t"
        << imf_fixdate(obj.last_modified) << "\t"
        << obj.etag.value_or("<-- no etag -->");
    return out;
}

uh::cluster::coro<void> list_bucket(directory& dir, const std::string& target) {
    if (target.empty()) {
        for (const auto& bucket : co_await dir.list_buckets()) {
            std::cout << bucket << "\n";
        }
        std::cout << "Accumulated directory size: " << co_await dir.data_size()
                  << "\n";
    } else {
        for (const auto& obj :
             co_await dir.list_objects(target, std::nullopt, std::nullopt)) {
            std::cout << obj << "\n";
        }
    }
}

uh::cluster::coro<void> make_bucket(directory& dir, const std::string& target) {
    co_await dir.put_bucket(target);
}

uh::cluster::coro<void> remove_bucket(directory& dir,
                                      const std::string& target) {
    co_await dir.delete_bucket(target);
}

int main(int argc, char** argv) {
    try {
        auto cfg = read_config(argc, argv);
        if (!cfg) {
            return 0;
        }

        boost::asio::io_context executor;

        uh::cluster::db::database db(cfg->database);
        auto handler = [](const std::exception_ptr& e) {
            if (e) {
                std::rethrow_exception(e);
            }
        };

        directory dir(db);

        switch (cfg->cmd) {
        case config::command::ls:
            boost::asio::co_spawn(executor, list_bucket(dir, cfg->target_ls),
                                  handler);
            break;
        case config::command::mkb:;
            boost::asio::co_spawn(executor, make_bucket(dir, cfg->target_mkb),
                                  handler);
            break;
        case config::command::rmb:;
            boost::asio::co_spawn(executor, remove_bucket(dir, cfg->target_rmb),
                                  handler);
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
