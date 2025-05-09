#include <common/utils/common.h>
#include <common/utils/misc.h>
#include <common/utils/time_utils.h>
#include <config/configuration.h>
#include <storage/interfaces/remote_storage.h>

#include <CLI/CLI.hpp>
#include <cctype>
#include <iostream>

using namespace uh::cluster;

struct config {
    enum class command { read, write };

    std::string hostname = "localhost";
    std::uint16_t port = 9200;

    command cmd = command::read;
    std::size_t host_id;
    std::size_t offset;
    std::size_t length;
    std::optional<std::string> output_file;
    bool no_output = false;

    std::string file;

    boost::log::trivial::severity_level log_level =
        boost::log::trivial::warning;
};

std::optional<::config> read_config(int argc, char** argv) {
    CLI::App app("UH storage CLI");
    argv = app.ensure_utf8(argv);

    ::config rv;

    app.add_option("--host", rv.hostname, "storage host name")
        ->default_val(rv.hostname);
    app.add_option("--port", rv.port, "storage port")->default_val(rv.port);

    auto* sub_read = app.add_subcommand("read", "read from given address");
    sub_read->add_option("host-id", rv.host_id, "upper 64 bit of address");
    sub_read->add_option("offset", rv.offset, "offset of address");
    sub_read->add_option("length", rv.length, "number of bytes to read");
    sub_read->add_option("-O,--output-file", rv.output_file,
                         "if set, dump to file");
    sub_read->add_flag("-n,--no-output", rv.no_output,
                       "do not output anything, only print throughput");

    auto* sub_write = app.add_subcommand("write", "write a buffer");
    sub_write->add_option("file", rv.file, "read buffer from file");

    uh::cluster::configure(app, rv.log_level);

    try {
        app.parse(argc, argv);
    } catch (const CLI::Success& e) {
        app.exit(e);
        return {};
    }

    uh::log::set_level(rv.log_level);
    if (sub_read->parsed()) {
        rv.cmd = ::config::command::read;
    } else if (sub_write->parsed()) {
        rv.cmd = ::config::command::write;
    }

    return rv;
}

uh::cluster::coro<void> read_addr(uh::cluster::storage_interface& svc,
                                  uh::cluster::uint128_t ptr,
                                  std::size_t length,
                                  std::optional<std::string> outfile,
                                  bool no_output) {
    timer t;
    auto data = co_await svc.read(ptr, length);
    auto time = t.passed();
    auto mb = length / MEBI_BYTE;

    std::cout << "read " << length << " bytes from " << ptr << " ("
              << (mb / time.count()) << " MB/s)\n";

    if (outfile) {
        std::ofstream out(*outfile);
        out.write(data.data(), data.size());
        std::cout << "contents have been written to " << *outfile << "\n";
    } else {

        if (!no_output) {
            std::size_t index = 0;
            do {
                std::cout << (ptr + index) << "    ";

                std::string text = "";

                auto count = std::min(16ul, data.size() - index);
                for (auto x = 0ul; x < count; ++x, ++index) {
                    std::cout << std::hex << std::setw(2) << std::setfill('0')
                              << static_cast<unsigned>(data[index] & 0xff)
                              << " ";

                    text += std::isprint(data[index])
                                ? std::string(1, data[index])
                                : ".";
                }

                std::string indent((16ul - count) * 3, ' ');
                std::cout << "    " << indent << "|" << text << "|"
                          << "\n";
            } while (index < data.size());
        }
    }
}

uh::cluster::coro<void> write_file(uh::cluster::storage_interface& svc,
                                   const std::string& file) {
    auto buffer = read_file(file);

    timer t;
    auto alloc = co_await svc.allocate(buffer.size());
    auto addr = co_await svc.write(alloc, buffer, {0});
    auto time = t.passed();

    auto mb = buffer.size() / MEBI_BYTE;

    std::cout << "wrote " << buffer.size() << " bytes from file '" << file
              << "' to address " << addr.to_string() << "\n";
    std::cout << "throughput: " << (mb / time.count()) << " MB/s\n";
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

        uh::cluster::remote_storage storage(
            uh::cluster::client(executor, cfg->hostname, cfg->port, 1));

        switch (cfg->cmd) {
        case ::config::command::read:
            boost::asio::co_spawn(
                executor,
                read_addr(storage,
                          uh::cluster::uint128_t(cfg->host_id, cfg->offset),
                          cfg->length, cfg->output_file, cfg->no_output),
                handler);
            break;
        case ::config::command::write:
            boost::asio::co_spawn(executor, write_file(storage, cfg->file),
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
