#include <boost/asio.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>
#include <iostream>
#include <set>

namespace asio = boost::asio;
using asio::use_awaitable;
using asio::ip::tcp;

namespace this_coro          = asio::this_coro;
constexpr auto nothrow_await = asio::as_tuple(use_awaitable);
using namespace asio::experimental::awaitable_operators;
using namespace std::literals::chrono_literals;

void once_in(size_t n, auto&& action) { // helper for reduced frequency logging
    static std::atomic_size_t counter_ = 0;
    if ((++counter_ % n) == 0) {
        if constexpr (std::is_invocable_v<decltype(action), size_t>)
            std::move(action)(counter_);
        else
            std::move(action)();
    }
}

asio::awaitable<void> timeout(auto duration) {
    asio::steady_timer timer(co_await this_coro::executor);
    timer.expires_after(duration);
    co_await timer.async_wait(nothrow_await);
}

asio::awaitable<void> server_session(tcp::socket socket) {
    try {
        for (std::array<char, 2000> data;;) {
            if (auto r = co_await (async_read(socket, asio::buffer(data), nothrow_await) || timeout(2ms));
                    r.index() == 1) {
                once_in(1000, [&] { std::cout << "server_session time out." << std::endl; });
            } else {
                auto [e, n] = std::get<0>(r);
                if (!e) {
                    once_in(1000, [&, n = n] {
                        std::cout << "server_session writing " << n << " bytes to "
                                  << socket.remote_endpoint() << std::endl;
                    });
                    if (n)
                        co_await async_write(socket, asio::buffer(data, n), use_awaitable);
                } else {
                    std::cout << e.message() << std::endl;
                }
            }
        }
    } catch (boost::system::system_error const& se) {
        std::cout << "server_session Exception: " << se.code().message() << std::endl;
    } catch (std::exception const& e) {
        std::cout << "server_session Exception: " << e.what() << std::endl;
    }

    std::cout << "server_session closed" << std::endl;
}

asio::awaitable<void> listener(uint16_t port) {
    for (tcp::acceptor acc(co_await this_coro::executor, {{}, port});;) {
        auto s = make_strand(acc.get_executor());
        co_spawn(                                                        //
                s,                                                           //
                server_session(co_await acc.async_accept(s, use_awaitable)), //
                asio::detached);
    }
}

asio::awaitable<void> client_session(uint16_t port) {
    try {
        tcp::socket socket(co_await this_coro::executor);
        co_await socket.async_connect({{}, port}, use_awaitable);

        for (std::array<char, 4024> data{0};;) {
            co_await (async_read(socket, asio::buffer(data), use_awaitable) || timeout(2ms));
            auto w = co_await async_write(socket, asio::buffer(data, 2000 /*SEHE?!*/), use_awaitable);

            once_in(1000, [&](size_t counter) {
                std::cout << "#" << counter << " wrote " << w << " bytes from " << socket.local_endpoint()
                          << std::endl;
            });
        }
    } catch (boost::system::system_error const& se) {
        std::cout << "client_session Exception: " << se.code().message() << std::endl;
    } catch (std::exception const& e) {
        std::cout << "client_session Exception: " << e.what() << std::endl;
    }

    std::cout << "client_session closed" << std::endl;
}

int main(int argc, char** argv) {
    auto flags = std::set<std::string_view>(argv + 1, argv + argc);
    bool server = flags.contains("server");
    bool client = flags.contains("client");

    asio::thread_pool pool(server ? 8 : 3);
    try {
        asio::signal_set signals(pool, SIGINT, SIGTERM);
        signals.async_wait([&](auto, auto) { pool.stop(); });

        if (server) {
            co_spawn(pool, listener(5555), asio::detached);
        }

        if (client) {
            co_spawn(make_strand(pool), client_session(5555), asio::detached);
            co_spawn(make_strand(pool), client_session(5555), asio::detached);
            co_spawn(make_strand(pool), client_session(5555), asio::detached);
        }

        std::this_thread::sleep_for(30s); // time limited for COLIRU
    } catch (std::exception const& e) {
        std::cout << "main Exception: " << e.what() << std::endl;
    }
}