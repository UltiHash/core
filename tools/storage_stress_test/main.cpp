#include "common/network/messenger.h"
#include "common/telemetry/log.h"
#include "common/types/common_types.h"
#include "common/utils/random.h"
#include "common/utils/time_utils.h"
#include <boost/asio/co_spawn.hpp>
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

size_t min_frag_size = 64ul * MEBI_BYTE;
size_t max_frag_size = 64ul * MEBI_BYTE;

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

size_t do_write(const unique_buffer<char>& buffer) {
    message_type type = STORAGE_WRITE_REQ;
    size_t length = buffer.size();
    std::vector<boost::asio::const_buffer> send_buffers{
        {&type, sizeof(type)},
        {&length, sizeof(length)},
        {buffer.data(), buffer.size()}};

    auto socket = borrow_connection();
    boost::asio::write(*socket, send_buffers);

    messenger_core::header h{};
    std::vector<boost::asio::mutable_buffer> recv_buffers{
        {&h.type, sizeof h.type}, {&h.size, sizeof h.size}};
    boost::asio::read(*socket, recv_buffers);
    if (h.type != SUCCESS) [[unlikely]] {
        throw std::runtime_error("unsuccessful write");
    }
    unique_buffer<char> recv_data(h.size);
    boost::asio::read(*socket,
                      boost::asio::buffer(recv_data.data(), recv_data.size()));

    return_connection(std::move(socket));
    return length;
}

size_t do_io(const params& ps) {

    std::random_device rd;
    std::mt19937 generator(rd());
    std::uniform_int_distribution<size_t> distribution(min_frag_size,
                                                       max_frag_size);

    size_t total_size = 0;
    for (size_t i = 0; i < ps.message_count; ++i) {
        size_t length = distribution(generator);
        unique_buffer<char> random_data(length);

        total_size += do_write(random_data);
    }

    return total_size;
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

    std::vector<size_t> io_sizes(ps.threads);
    std::vector<std::exception_ptr> exceptions(ps.threads);

    timer tt;

    for (int i = 0; i < ps.threads; ++i) {
        threads.emplace_back([&ps, &io_sizes, &exceptions, i]() {
            try {
                io_sizes[i] = do_io(ps);
            } catch (const std::exception&) {
                exceptions[i] = std::current_exception();
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    for (const auto& e : exceptions) {
        if (e)
            std::rethrow_exception(e);
    }

    const auto accumulated_size =
        std::accumulate(io_sizes.cbegin(), io_sizes.cend(), 0.0);

    const std::chrono::duration<double> duration = tt.passed();
    const auto size = accumulated_size / static_cast<double>(MEBI_BYTE);
    const auto bandwidth = size / duration.count();
    LOG_INFO() << "Wrote " << size << " MB";
    LOG_INFO() << "Operation duration " << duration.count() << " s";
    LOG_INFO() << "Operation bandwidth " << bandwidth << " MB/s";
}
