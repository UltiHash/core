#include "common/network/messenger.h"
#include "common/types/common_types.h"
#include "common/utils/random.h"
#include "common/utils/time_utils.h"
#include <CLI/CLI.hpp>
#include <deduplicator/interfaces/remote_deduplicator.h>
#include <filesystem>
#include <fstream>
#include <future>
#include <iostream>
#include <random>

using namespace uh::cluster;

struct config {
    std::string host = "127.0.0.1";
    uint16_t port = 9300;
    std::size_t threads = 4;
    std::size_t connections = 16;
    std::size_t jobs = 8;
    size_t min_data_size = 32ul * MEBI_BYTE;
    size_t max_data_size = 32ul * MEBI_BYTE;
    size_t messages = 32;
    std::string generator = "random";
};

std::optional<::config> read_config(int argc, char** argv) {
    CLI::App app("UH deduplicator stress test");
    argv = app.ensure_utf8(argv);

    app.description(
        "Send a bunch of deduplication requests to a deduplicator. "
        "The test runs N jobs, each sending a given number of messages. ");

    ::config rv;

    app.add_option("--host,-H", rv.host, "DD host name")->default_val(rv.host);
    app.add_option("--port,-p", rv.port, "DD port")->default_val(rv.port);
    app.add_option("--threads,-t", rv.threads, "number of I/O threads")
        ->default_val(rv.threads);
    app.add_option("--connections,-c", rv.connections, "number of connections")
        ->default_val(rv.connections);
    app.add_option("--messages,-n", rv.messages, "number of messages to send")
        ->default_val(rv.messages);
    app.add_option("--jobs,-j", rv.jobs, "number of jobs")
        ->default_val(rv.jobs);
    app.add_option("--min", rv.min_data_size, "minimum size of message")
        ->default_val(rv.min_data_size);
    app.add_option("--max", rv.max_data_size, "maximum size of message")
        ->default_val(rv.max_data_size);
    app.add_option("--generator", rv.generator,
                   "data generator (random or fixed)")
        ->default_val(rv.generator);

    try {
        app.parse(argc, argv);
    } catch (const CLI::Success& e) {
        app.exit(e);
        return {};
    }

    return rv;
}

std::string fixed_string(std::size_t length) {
    std::string pattern = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

    std::string rv;
    rv.reserve(length);

    while (rv.size() < length) {
        rv.append(pattern, 0, std::min(pattern.size(), length - rv.size()));
    }

    return rv;
}

std::vector<std::vector<std::string>> gen_fixed(const config& cfg) {

    std::random_device rd;
    std::mt19937 generator(rd());
    std::uniform_int_distribution<size_t> distribution(cfg.min_data_size,
                                                       cfg.max_data_size);

    std::vector<std::vector<std::string>> data;
    data.reserve(cfg.jobs);

    for (std::size_t t = 0; t < cfg.jobs; t++) {
        data.emplace_back();
        data.back().reserve(cfg.messages);
        for (size_t i = 0; i < cfg.messages; ++i) {
            size_t length = distribution(generator);
            data.back().emplace_back(fixed_string(length));
        }
    }

    return data;
}

std::vector<std::vector<std::string>> gen_random(const config& cfg) {

    std::random_device rd;
    std::mt19937 generator(rd());
    std::uniform_int_distribution<size_t> distribution(cfg.min_data_size,
                                                       cfg.max_data_size);

    std::vector<std::vector<std::string>> data;
    data.reserve(cfg.jobs);

    for (std::size_t t = 0; t < cfg.jobs; t++) {
        data.emplace_back();
        data.back().reserve(cfg.messages);
        for (size_t i = 0; i < cfg.messages; ++i) {
            size_t length = distribution(generator);
            data.back().emplace_back(random_string(length));
        }
    }

    return data;
}

std::vector<std::vector<std::string>> generate_data(const config& cfg) {
    if (cfg.generator == "random") {
        return gen_random(cfg);
    }

    if (cfg.generator == "fixed") {
        return gen_fixed(cfg);
    }

    throw std::runtime_error("unsupported generator: " + cfg.generator);
}

coro<void> do_io(deduplicator_interface& dd,
                 const std::vector<std::string>& data) {
    for (const auto& data : data) {
        co_await dd.deduplicate(data);
    }
}

int main(int argc, char** argv) {

    try {
        auto cfg = ::read_config(argc, argv);
        if (!cfg) {
            return 0;
        }

        boost::asio::io_context ioc;
        auto handler = [](const std::exception_ptr& e) {
            if (e) {
                std::rethrow_exception(e);
            }
        };

        remote_deduplicator dd(
            client(ioc, cfg->host, cfg->port, cfg->connections));
        std::list<std::thread> threads;

        std::cout << "generating random data: " << cfg->messages
                  << " for every job(" << cfg->jobs << "), each between "
                  << cfg->min_data_size << " and " << cfg->max_data_size
                  << " bytes in size.\n";
        std::size_t total = cfg->messages * cfg->jobs *
                            ((cfg->min_data_size + cfg->max_data_size) / 2);
        std::cout << "average total size: " << total << "\n";

        const auto data = generate_data(*cfg);

        std::cout << "generation done, starting upload\n";
        timer tt;

        for (std::size_t i = 0; i < cfg->jobs; ++i) {
            boost::asio::co_spawn(ioc, do_io(dd, data[i]), handler);
        }

        for (std::size_t i = 0; i < cfg->threads; ++i) {
            threads.emplace_back([&ioc]() { ioc.run(); });
        }

        for (auto& t : threads) {
            t.join();
        }

        auto passed = tt.passed();
        std::size_t data_size = std::accumulate(
            data.begin(), data.end(), 0ull, [](auto val, auto& vec) {
                return val + std::accumulate(vec.begin(), vec.end(), 0ull,
                                             [](auto val, auto& s) {
                                                 return val + s.size();
                                             });
            });

        std::cout << "time passed: " << passed << "s\n";
        std::cout << "data uploaded: " << data_size << " bytes ("
                  << data_size / MEBI_BYTE << " MB)\n";
        std::cout << "throughput: "
                  << ((data_size / MEBI_BYTE) / passed.count()) << " MB/s\n";

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
