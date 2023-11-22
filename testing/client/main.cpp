//
// Created by massi on 11/20/23.
//
#include <iostream>
#include <fstream>
#include "network/messenger.h"
#include "common/common_types.h"
#include "common/log.h"
#include <filesystem>
#include <boost/asio/co_spawn.hpp>
#include <future>

using namespace uh::cluster;

struct params {
    std::string address;
    long port {};
    message_type req_type {};
    std::filesystem::path file_path;
};

std::promise <ospan <char>> response;

params get_params (int argc, char* args[]) {

    if (argc != 5) {
        throw std::invalid_argument ("Wrong number of parameters");
    }

     return {
            .address = std::string {args[1]},
            .port = strtol (args[2], nullptr, 10),
            .req_type = static_cast <message_type> (strtol(args[3], nullptr, 10)),
            .file_path = {args[4]},
    };
}

coro <void> perform_operation (messenger& m, message_type type, std::span <char> data) {
    co_await m.send(type, data);
    const auto h = co_await m.recv_header();
    ospan <char> buf (h.size);
    m.register_read_buffer(buf);
    co_await m.recv_buffers(h);
    response.set_value(std::move (buf));
}

std::string dump_usage () {
    return {
        "Usage: <executable> <server-address> <server-port> <request-type> <data-file-path>"
    };
}

int main (int argc, char* args[]) {

    // executable server port type file


    params ps;

    try {
        ps = get_params(argc, args);
    } catch (std::exception& e) {
        LOG_ERROR () << "Error in parameters:";
        LOG_ERROR() << e.what();
        LOG_ERROR() << dump_usage ();
    }

    auto ioc = std::make_shared <boost::asio::io_context> ();
    messenger m (ioc, ps.address, static_cast <int> (ps.port));

    LOG_INFO() << "Connected to the server";

    std::fstream f (ps.file_path);
    ospan <char> buf (std::filesystem::file_size (ps.file_path));
    f.read(buf.data.get(), buf.size);

    LOG_INFO() << "Read data from file " << ps.file_path << " of size " << buf.size / static_cast <double> (1024ul * 1024ul);

    std::chrono::time_point <std::chrono::steady_clock> timer;
    const auto start = std::chrono::steady_clock::now();

    boost::asio::co_spawn (*ioc, perform_operation(m, ps.req_type, buf.get_span()), [] (const std::exception_ptr& eptr) {
        if (eptr) {
            try {
                std::rethrow_exception(eptr);
            } catch (std::exception& e) {
                LOG_ERROR() << "Operation error: " << e.what();
            }
        }
    });
    ioc->run();

    const auto stop = std::chrono::steady_clock::now ();
    const std::chrono::duration <double> duration = stop - start;
    const auto size = buf.size / static_cast <double> (1024ul * 1024ul);
    const auto bandwidth = size / duration.count();
    LOG_INFO() << "Operation duration " << duration.count() << " s";
    LOG_INFO() << "Operation bandwidth " << bandwidth << " MB/s";

    const auto resp = response.get_future().get();
    LOG_INFO() << "Operation response of size " << resp.size;

}