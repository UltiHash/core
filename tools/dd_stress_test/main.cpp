#include "common/network/messenger.h"
#include "common/telemetry/log.h"
#include "common/types/common_types.h"
#include "common/utils/random.h"
#include <filesystem>
#include <fstream>
#include <future>
#include <random>

using namespace uh::cluster;

struct params {
    std::string address;
    long port{};
    long threads{};
    long conns{};
    size_t message_count{};
};

size_t min_data_size = 128ul * MEBI_BYTE;
size_t max_data_size = 512ul * MEBI_BYTE;

boost::asio::io_context ioc;
std::deque<std::unique_ptr<boost::asio::ip::tcp::socket>> sockets;
std::mutex m;
std::condition_variable cv;

void create_connections(const params& ps) {
    boost::asio::ip::tcp::endpoint endpoint(
        boost::asio::ip::address::from_string(ps.address), ps.port);

    for (int i = 0; i < ps.conns; ++i) {
        sockets.emplace_back(
            std::make_unique<boost::asio::ip::tcp::socket>(ioc));
        sockets.back()->connect(endpoint);
    }
}

auto borrow_connection() {
    std::unique_lock<std::mutex> l(m);
    cv.wait(l, []() { return !sockets.empty(); });
    auto s = std::move(sockets.front());
    sockets.pop_front();
    return s;
}

void return_connection(auto&& s) {
    std::lock_guard<std::mutex> l(m);
    sockets.push_back(std::move(s));
    cv.notify_one();
}

std::vector<std::vector<std::string>> generate_data(const params& ps) {

    std::random_device rd;
    std::mt19937 generator(rd());
    std::uniform_int_distribution<size_t> distribution(min_data_size,
                                                       max_data_size);

    std::vector<std::vector<std::string>> random_data;
    random_data.reserve(ps.threads);

    for (int t = 0; t < ps.threads; t++) {
        random_data.emplace_back();
        random_data.back().reserve(ps.message_count);
        for (size_t i = 0; i < ps.message_count; ++i) {
            size_t length = distribution(generator);
            random_data.back().emplace_back(random_string(length));
        }
    }

    return random_data;
}

std::pair<size_t, size_t> do_io(const std::vector<std::string>& random_data) {

    size_t total_size = 0;
    size_t effective_size = 0;

    for (const auto& data : random_data) {
        message_type type = DEDUPLICATOR_REQ;
        const auto length = data.length();
        std::vector<boost::asio::const_buffer> send_buffers{
            {&type, sizeof(type)},
            {&length, sizeof(length)},
            {data.data(), data.size()}};

        auto socket = borrow_connection();
        boost::asio::write(*socket, send_buffers);

        messenger_core::header h{};
        std::vector<boost::asio::mutable_buffer> recv_buffers{
            {&h.type, sizeof h.type}, {&h.size, sizeof h.size}};
        boost::asio::read(*socket, recv_buffers);

        dedupe_response dedupe_resp;
        dedupe_resp.addr.allocate_for_serialized_data(
            h.size - sizeof(dedupe_resp.effective_size));
        std::vector<boost::asio::mutable_buffer> buffers{
            boost::asio::buffer(&dedupe_resp.effective_size,
                                sizeof dedupe_resp.effective_size),
            boost::asio::buffer(dedupe_resp.addr.pointers),
            boost::asio::buffer(dedupe_resp.addr.sizes),
        };

        boost::asio::read(*socket, buffers);
        if (h.type != SUCCESS) [[unlikely]] {
            throw std::runtime_error("unsuccessful integration");
        }

        return_connection(std::move(socket));
        total_size += length;
        effective_size += dedupe_resp.effective_size;
    }

    return {total_size, effective_size};
}

params get_params(int argc, char* args[]) {

    if (argc != 6) {
        throw std::invalid_argument("Wrong number of parameters");
    }

    return {
        .address = std::string{args[1]},
        .port = strtol(args[2], nullptr, 10),
        .threads = strtol(args[3], nullptr, 10),
        .conns = strtol(args[4], nullptr, 10),
        .message_count = strtoul(args[5], nullptr, 10),
    };
}

std::string dump_usage() {
    return {"Usage: <executable> <server-bind_address> <server-port> "
            "<threads-count> <connection-count> <message-count>"};
}

int main(int argc, char* args[]) {

    params ps;

    try {
        ps = get_params(argc, args);
    } catch (std::exception& e) {
        LOG_ERROR() << "Error in parameters:";
        LOG_ERROR() << e.what();
        LOG_ERROR() << dump_usage();
        exit(1);
    }

    create_connections(ps);

    std::vector<std::thread> threads;
    threads.reserve(ps.threads);

    std::vector<std::pair<size_t, size_t>> io_sizes(ps.threads);
    std::vector<std::exception_ptr> exceptions(ps.threads);

    const auto random_data = generate_data(ps);

    std::chrono::time_point<std::chrono::steady_clock> timer;
    const auto start = std::chrono::steady_clock::now();

    for (int i = 0; i < ps.threads; ++i) {
        threads.emplace_back([&random_data, &io_sizes, &exceptions, i]() {
            try {
                io_sizes[i] = do_io(random_data[i]);
            } catch (const std::exception&) {
                exceptions[i] = std::current_exception();
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    for (const auto& e : exceptions) {
        if (e) {
            try {
                std::rethrow_exception(e);
            } catch (const std::exception& e) {
                LOG_ERROR() << "Error occurred in threads: " << e.what();
                exit(1);
            }
        }
    }

    const auto accumulated_total_size =
        std::accumulate(io_sizes.cbegin(), io_sizes.cend(), 0.0,
                        [](auto sum, auto& v) { return sum + v.first; });

    const auto accumulated_eff_size =
        std::accumulate(io_sizes.cbegin(), io_sizes.cend(), 0.0,
                        [](auto sum, auto& v) { return sum + v.second; });

    const auto stop = std::chrono::steady_clock::now();
    const std::chrono::duration<double> duration = stop - start;
    const auto total_size =
        accumulated_total_size / static_cast<double>(MEBI_BYTE);
    const auto eff_size = accumulated_eff_size / static_cast<double>(MEBI_BYTE);
    const auto bandwidth = total_size / duration.count();
    LOG_INFO() << "Integrated " << total_size << " MB";
    LOG_INFO() << "Effective size " << eff_size << " MB";
    LOG_INFO() << "Deduplication ratio " << 1.0 - eff_size / total_size
               << " MB";
    LOG_INFO() << "Operation duration " << duration.count() << " s";
    LOG_INFO() << "Operation bandwidth " << bandwidth << " MB/s";
}
