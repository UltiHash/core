#include "common/coroutines/context.h"
#include "common/db/db.h"
#include "config/configuration.h"
#include "entrypoint/directory.h"
#include "entrypoint/formats.h"
#include "storage/interfaces/remote_storage.h"

#include <CLI/CLI.hpp>
#include <cctype>
#include <iostream>

using uh::cluster::directory;
using uh::cluster::imf_fixdate;
using uh::cluster::object;

struct config {
    enum class command { read };

    command cmd = command::read;
    std::size_t host_id;
    std::size_t offset;
    std::size_t length;
    std::string hostname = "localhost";
    std::uint16_t port = 9200;
};

std::optional<config> read_config(int argc, char** argv) {
    CLI::App app("UH storage CLI");
    argv = app.ensure_utf8(argv);

    config rv;

    app.add_option("--host", rv.hostname, "storage host name")
        ->default_val(rv.hostname);
    app.add_option("--port", rv.port, "storage port")->default_val(rv.port);
    auto* sub_read = app.add_subcommand("read", "read from a given address");
    sub_read->add_option("host-id", rv.host_id, "upper 64 bit of address");
    sub_read->add_option("offset", rv.offset, "offset of address");
    sub_read->add_option("length", rv.length, "number of bytes to read");

    try {
        app.parse(argc, argv);
    } catch (const CLI::Success& e) {
        app.exit(e);
        return {};
    }

    if (sub_read->parsed()) {
        rv.cmd = config::command::read;
    }

    return rv;
}

uh::cluster::coro<void> read_addr(uh::cluster::storage_interface& svc,
                                  const uh::cluster::uint128_t& ptr,
                                  std::size_t length) {
    uh::cluster::context ctx;

    auto data = co_await svc.read(ctx, ptr, length);

    std::cout << "read " << length << " bytes from " << ptr << ":\n";

    std::size_t index = 0;
    do {
        std::cout << (ptr + index) << "    ";

        std::string text = "";

        auto count = std::min(16ul, data.size() - index);
        for (unsigned x = 0; x < count; ++x, ++index) {
            std::cout << std::hex << std::setw(2) << std::setfill('0')
                      << static_cast<int>(data[index]) << " ";

            text +=
                std::isprint(data[index]) ? std::string(1, data[index]) : ".";
        }

        std::string indent((16ul - count) * 3, ' ');
        std::cout << "    " << indent << "|" << text << "|"
                  << "\n";
    } while (index < data.size());
}

int main(int argc, char** argv) {
    try {
        auto cfg = read_config(argc, argv);
        if (!cfg) {
            return 0;
        }

        boost::asio::io_context executor;
        auto handler = [](const std::exception_ptr& e) {
            if (e) {
                std::rethrow_exception(e);
            }
        };

        uh::cluster::remote_storage storage(
            uh::cluster::client(executor, cfg->hostname, cfg->port, 1));

        switch (cfg->cmd) {
        case config::command::read:
            boost::asio::co_spawn(
                executor,
                read_addr(storage,
                          uh::cluster::uint128_t(cfg->host_id, cfg->offset),
                          cfg->length),
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
