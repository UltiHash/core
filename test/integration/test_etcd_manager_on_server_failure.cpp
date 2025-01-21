// #include <boost/asio/executor_work_guard.hpp>
// #define BOOST_TEST_MODULE "etcd manager tests"
//
// #include <boost/asio.hpp>
// #include <boost/asio/experimental/co_spawn.hpp>
// #include <boost/asio/io_context.hpp>
// #include <boost/asio/ip/tcp.hpp>
// #include <boost/asio/steady_timer.hpp>
// #include <boost/system/error_code.hpp>
// #include <boost/test/unit_test.hpp>
// #include <chrono>
// #include <iostream>
// #include <thread>
//
// namespace asio = boost::asio;
// using tcp = asio::ip::tcp;
//
// // 서버 연결 실패를 시뮬레이션하는 프록시
// class proxy {
// public:
//     proxy(asio::io_context& io_context, const std::string& listen_address,
//           short listen_port, const std::string& forward_address,
//           short forward_port)
//         : acceptor_(io_context,
//                     tcp::endpoint(asio::ip::make_address(listen_address),
//                                   listen_port)),
//           forward_address_(forward_address),
//           forward_port_(forward_port),
//           m_io_context(io_context) {}
//
//     void start() {
//         asio::co_spawn(
//             m_io_context, [this] { return this->accept_connections(); },
//             asio::detached);
//     }
//
// private:
//     asio::ip::tcp::acceptor acceptor_;
//     std::string forward_address_;
//     short forward_port_;
//     asio::io_context& m_io_context;
//
//     // 클라이언트와 서버 연결 처리
//     asio::awaitable<void> accept_connections() {
//         while (true) {
//             tcp::socket client_socket(m_io_context);
//             try {
//                 // 클라이언트 연결 대기
//                 co_await acceptor_.async_accept(client_socket,
//                                                 asio::use_awaitable);
//                 std::cout << "Client connected\n";
//
//                 // 연결된 클라이언트와 서버 간 포트 포워딩
//                 co_spawn(
//                     m_io_context,
//                     [this, client_socket = std::move(client_socket)]()
//                     mutable {
//                         return this->handle_client(std::move(client_socket));
//                     },
//                     asio::detached);
//             } catch (const std::exception& e) {
//                 std::cerr << "Error accepting connection: " << e.what()
//                           << std::endl;
//             }
//         }
//     }
//
//     // 클라이언트와 서버 간 데이터 포워딩
//     asio::awaitable<void> handle_client(tcp::socket client_socket) {
//         try {
//             // 서버에 연결
//             tcp::socket server_socket(m_io_context);
//             co_await server_socket.async_connect(
//                 tcp::endpoint(asio::ip::make_address(forward_address_),
//                               forward_port_),
//                 asio::use_awaitable);
//             std::cout << "Server connected\n";
//
//             // 데이터 전송 및 수신
//             asio::steady_timer timer(m_io_context);
//             char data[1024];
//
//             while (true) {
//                 // 클라이언트로부터 데이터 읽기
//                 std::size_t n = co_await client_socket.async_read_some(
//                     asio::buffer(data), asio::use_awaitable);
//
//                 // 서버로 데이터 전송
//                 co_await asio::async_write(server_socket, asio::buffer(data,
//                 n),
//                                            asio::use_awaitable);
//
//                 // 서버로부터 응답 읽기
//                 n = co_await
//                 server_socket.async_read_some(asio::buffer(data),
//                                                            asio::use_awaitable);
//
//                 // 클라이언트로 응답 전달
//                 co_await asio::async_write(client_socket, asio::buffer(data,
//                 n),
//                                            asio::use_awaitable);
//             }
//         } catch (const std::exception& e) {
//             std::cerr << "Error handling client: " << e.what() << std::endl;
//         }
//     }
// };
//
// // 테스트를 위한 Fixture 클래스
// struct proxy_fixture {
//     proxy_fixture()
//         : m_io_context(),
//           work{asio::make_work_guard(m_io_context)},
//           io_thread{[this]() { m_io_context.run(); }},
//           proxy_(m_io_context, "127.0.0.1", 9000, "127.0.0.1", 8000) {
//         proxy_.start();
//     }
//
//     ~proxy_fixture() { m_io_context.stop(); }
//
//     void run_io_context() { m_io_context.run(); }
//
//     asio::io_context m_io_context;
//     asio::io_context::work work;
//     std::jthread io_thread;
//     proxy proxy_;
// };
//
// // 실제 테스트에서 사용할 예시
// BOOST_FIXTURE_TEST_SUITE(ProxyTests, proxy_fixture)
//
// BOOST_AUTO_TEST_CASE(HandleClientConnection) {
//     // 실제 서버와 포트 포워딩 동작을 테스트할 수 있습니다.
//     // 여기에서는 단순히 proxy_fixture로 설정된 서버로 연결 시도를 합니다.
//     std::jthread io_thread([this]() { run_io_context(); });
//
//     std::this_thread::sleep_for(
//         std::chrono::seconds(2)); // 실제 연결 시뮬레이션을 위해 기다립니다.
//
//     BOOST_CHECK(true); // 실제 테스트 로직을 구현하십시오.
// }
//
// BOOST_AUTO_TEST_SUITE_END()

#define BOOST_TEST_MODULE "etcd manager tests"

#include "common/etcd/utils.h"
#include "common/telemetry/log.h"
#include "fakeit/fakeit.hpp"
#include <cstdlib>

#include <boost/test/unit_test.hpp>

using namespace fakeit;
using namespace std::chrono_literals;

namespace uh::cluster {

class callback_interface {
public:
    virtual ~callback_interface() = default;
    virtual void handle_state_changes(const etcd::Response& response) = 0;
};

class etcd_manager_fixture {
public:
    void setup() {
        auto log_config = log::config{
            .sinks = {log::sink_config{.type = log::sink_type::cout,
                                       .level = boost::log::trivial::debug,
                                       .service_role = DEDUPLICATOR_SERVICE}}};
        log::init(log_config);

        std::system("sudo systemctl start etcd");
        std::this_thread::sleep_for(1s);
    }

    etcd_manager_fixture()
        : etcd{{}, lease_seconds} {
        When(Method(mock, handle_state_changes))
            .AlwaysDo([](const etcd::Response&) {});
    }

    ~etcd_manager_fixture() {
        etcd.clear_all();
        std::this_thread::sleep_for(100ms);
    }

protected:
    static constexpr int lease_seconds = 30;
    etcd_config cfg;
    etcd_manager etcd;
    etcd::Response response;
    Mock<callback_interface> mock;
};

class etcd_client_fixture {
public:
    etcd_client_fixture()
        : etcd_address{"http://127.0.0.1:2379"},
          etcd_client{etcd_address} {
        When(Method(mock, handle_state_changes))
            .AlwaysDo([](const etcd::Response&) {});
    }
    void setup() { std::system("sudo systemctl start etcd"); }
    void teardown() { etcd_client.rmdir("/", true); }

protected:
    std::string etcd_address;
    etcd::SyncClient etcd_client;
    etcd_config cfg;
    etcd::Response response;
    Mock<callback_interface> mock;
};

BOOST_FIXTURE_TEST_SUITE(a_etcd_client, etcd_client_fixture)

BOOST_AUTO_TEST_CASE(read_after_server_restarted) {
    etcd_client.put("/foo/bar", "1");

    BOOST_CHECK_EQUAL(std::system("sudo systemctl stop etcd"), 0);
    std::this_thread::sleep_for(1s);
    BOOST_CHECK_EQUAL(std::system("sudo systemctl start etcd"), 0);
    std::this_thread::sleep_for(5s);

    auto resp = etcd_client.get("/foo/bar");

    BOOST_TEST(resp.is_ok() == true);
    BOOST_TEST(resp.value().as_string() == "1");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_FIXTURE_TEST_SUITE(when_etcd_has_system_failure_a_etcd_manager,
                         etcd_manager_fixture)

BOOST_AUTO_TEST_CASE(
    recovers_previous_attached_watchers_to_watch_change_well_after_etcd_restarts) {
    auto wg = etcd.watch("/test_1",
                         [&cb = mock.get()](const etcd::Response& response) {
                             cb.handle_state_changes(response);
                         });
    BOOST_CHECK_EQUAL(std::system("sudo systemctl stop etcd"), 0);
    std::this_thread::sleep_for(1s);
    BOOST_CHECK_EQUAL(std::system("sudo systemctl start etcd"), 0);
    std::this_thread::sleep_for(5s);

    etcd.put("/test_1/a0", "172.0.0.1");
    std::this_thread::sleep_for(100ms);

    Verify(Method(mock, handle_state_changes)).Exactly(1);
}

BOOST_AUTO_TEST_CASE(reads_the_previously_written_value) {
    const auto value = std::string("172.0.0.1");
    etcd.put("/test_1/a0", value);

    BOOST_CHECK_EQUAL(std::system("sudo systemctl stop etcd"), 0);
    std::this_thread::sleep_for(1s);
    BOOST_CHECK_EQUAL(std::system("sudo systemctl start etcd"), 0);
    std::this_thread::sleep_for(5s);

    auto read = etcd.get("/test_1/a0");
    BOOST_TEST(value == read);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace uh::cluster
