#define BOOST_TEST_MODULE "disk-cache's http body tests"

#include <boost/test/unit_test.hpp>

#include <util/dedupe_fixture.h>

#include <proxy/cache/disk/body.h>

#include <common/utils/random.h>
#include <entrypoint/http/stream.h>
#include <proxy/cache/disk/body.h>

#include <boost/beast/http/parser.hpp>
#include <boost/beast/http/string_body.hpp>

using namespace boost::beast::http;

namespace uh::cluster::proxy::cache::disk {

BOOST_FIXTURE_TEST_SUITE(a_disk_cache_body, dedupe_fixture)

BOOST_AUTO_TEST_CASE(supports_read) {
    std::string data = random_string(64);
    std::string header = "POST /upload HTTP/1.1\r\n"
                         "Host: localhost\r\n"
                         "Content-Length: " +
                         std::to_string(data.size()) + "\r\n\r\n";
    std::string req = header + data;

    std::cout << req << std::endl;

    // Set up TCP sockets
    boost::asio::ip::tcp::acceptor acceptor(m_ioc,
                                            {boost::asio::ip::tcp::v4(), 0});
    auto endpoint = acceptor.local_endpoint();

    boost::asio::ip::tcp::socket server_sock(m_ioc);
    boost::asio::ip::tcp::socket client_sock(m_ioc);

    client_sock.connect(endpoint);
    acceptor.accept(server_sock);

    // Client writes HTTP request
    auto written_size =
        boost::asio::write(client_sock, boost::asio::buffer(req));

    BOOST_TEST(written_size == req.size());

    // Read header
    ep::http::socket_stream stream(server_sock);
    auto buffer = boost::asio::co_spawn(m_ioc, stream.read_until("\r\n\r\n"),
                                        boost::asio::use_future)
                      .get();

    BOOST_TEST(!buffer.empty());
    BOOST_TEST(buffer.size() == header.size());
    BOOST_TEST(std::string(buffer.data(), buffer.size()) == header);

    boost::beast::http::request_parser<boost::beast::http::empty_body> parser;
    parser.body_limit((std::numeric_limits<std::uint64_t>::max)());
    boost::beast::error_code ec;
    // parser.put(boost::asio::buffer(buffer), ec);
    parser.put(boost::asio::buffer(header.data(), header.size()), ec);

    auto res = parser.get();

    BOOST_TEST(parser.is_header_done());

    std::size_t content_length = std::stoul(res.at("Content-Length"));

    BOOST_TEST(content_length == data.size());

    // 6. Read body using async_read and reader_body
    writer w(data_view);
    boost::asio::co_spawn(m_ioc, async_read(stream, w, content_length),
                          boost::asio::use_future)
        .get();

    // 7. Verify w was stored and can be read back
    auto objh = w.get_object_handle();
    BOOST_TEST(objh.data_size() == data.size());

    std::vector<char> buf(data.size());
    boost::asio::co_spawn(
        m_ioc,
        data_view.read_address(objh.get_address(),
                               std::span<char>{buf.data(), buf.size()}),
        boost::asio::use_future)
        .get();
    BOOST_TEST(std::string(buf.data(), buf.size()) == data);
}

BOOST_AUTO_TEST_CASE(supports_write) {
    std::string data = random_string(64);
    std::string header = "POST /download HTTP/1.1\r\n"
                         "Host: localhost\r\n"
                         "Content-Length: " +
                         std::to_string(data.size()) + "\r\n\r\n";
    std::string expected_response = header + data;
    std::cout << expected_response << std::endl;

    auto addr = boost::asio::co_spawn(
                    m_ioc,
                    data_view.write(
                        std::span<const char>{data.data(), data.size()}, {0}),
                    boost::asio::use_future)
                    .get();
    auto objh = std::make_shared<object_handle>(std::move(addr));

    BOOST_TEST(objh->data_size() == data.size());
    reader r(data_view, std::move(objh));

    // Set up TCP sockets
    boost::asio::ip::tcp::acceptor acceptor(m_ioc,
                                            {boost::asio::ip::tcp::v4(), 0});
    auto endpoint = acceptor.local_endpoint();
    boost::asio::ip::tcp::socket server_sock(m_ioc);
    boost::asio::ip::tcp::socket client_sock(m_ioc);
    client_sock.connect(endpoint);
    acceptor.accept(server_sock);

    ep::http::socket_stream stream(client_sock);

    // Client writes HTTP response header
    auto written_size =
        boost::asio::write(client_sock, boost::asio::buffer(header));
    BOOST_TEST(written_size == header.size());

    // Client writes r using async_write and writer_body
    boost::asio::co_spawn(m_ioc, async_write<16_KiB>(stream, r),
                          boost::asio::use_future)
        .get();

    // Server reads full response
    std::string received;
    received.resize(expected_response.size());
    auto read_size = boost::asio::read(
        server_sock, boost::asio::buffer(received.data(), received.size()));
    BOOST_TEST(read_size == expected_response.size());
    BOOST_TEST(received == expected_response);
}

BOOST_AUTO_TEST_CASE(goes_with_relay_store_body) {
    using namespace boost::asio;
    using namespace boost::beast::http;

    ip::tcp::acceptor acceptor(m_ioc, ip::tcp::endpoint(ip::tcp::v4(), 0));
    ip::tcp::endpoint endpoint = acceptor.local_endpoint();

    ip::tcp::socket server_socket(m_ioc);
    ip::tcp::socket client_socket(m_ioc);

    std::thread server_thread([&] { acceptor.accept(server_socket); });

    client_socket.connect(endpoint);
    server_thread.join();

    std::string body = random_string(8_KiB + 17);
    std::string header = "POST /upload HTTP/1.1\r\n"
                         "Host: example.com\r\n"
                         "User-Agent: test\r\n"
                         "Content-Length: " +
                         std::to_string(body.size()) + "\r\n\r\n";
    auto raw_message = header + body;

    write(client_socket, buffer(header));
    write(client_socket, buffer(body));

    boost::beast::flat_buffer b;

    parser<true, double_buffer_body> p;
    serializer<true, double_buffer_body, fields> sr{p.get()};

    writer w(data_view);

    co_spawn(
        m_ioc,
        [&]() -> coro<void> {
            auto n = co_await async_read_header(server_socket, b, p);
            auto m = co_await async_write_store_header(server_socket, sr, w);
            BOOST_TEST(n == m);
            co_await async_relay_store_body<2_KiB>(server_socket, server_socket,
                                                   b, p, sr, w);
        },
        boost::asio::use_future)
        .get();

    boost::system::error_code ec;
    std::vector<char> recv_buf(16 * 1024);
    size_t n = client_socket.read_some(buffer(recv_buf), ec);
    std::string output_str(recv_buf.data(), n);

    BOOST_CHECK_NE(output_str.find("POST /upload HTTP/1.1"), std::string::npos);
    BOOST_CHECK_NE(output_str.find("Host: example.com"), std::string::npos);
    BOOST_CHECK_NE(output_str.find("User-Agent: test"), std::string::npos);

    // auto body_pos = output_str.find("\r\n\r\n");
    // BOOST_REQUIRE(body_pos != std::string::npos);
    // body_pos += 4;
    // std::string_view received_body(&recv_buf[body_pos], n);
    BOOST_TEST(output_str ==
               std::string_view(raw_message.data(), raw_message.size()));

    auto objh = w.get_object_handle();
    BOOST_TEST(objh.data_size() == raw_message.size());

    std::vector<char> buf(raw_message.size());
    boost::asio::co_spawn(
        m_ioc,
        data_view.read_address(objh.get_address(),
                               std::span<char>{buf.data(), buf.size()}),
        boost::asio::use_future)
        .get();
    BOOST_TEST(std::string(buf.data(), buf.size()) == raw_message);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_CASE(test_relay_body) {
    using namespace boost::asio;
    using namespace boost::beast::http;

    io_context ioc;
    ip::tcp::acceptor acceptor(ioc, ip::tcp::endpoint(ip::tcp::v4(), 0));
    ip::tcp::endpoint endpoint = acceptor.local_endpoint();

    ip::tcp::socket server_socket(ioc);
    ip::tcp::socket client_socket(ioc);

    std::thread server_thread([&] { acceptor.accept(server_socket); });

    client_socket.connect(endpoint);
    server_thread.join();

    std::string body = random_string(8_KiB + 17);
    std::string header = "POST /upload HTTP/1.1\r\n"
                         "Host: example.com\r\n"
                         "User-Agent: test\r\n"
                         "Content-Length: " +
                         std::to_string(body.size()) + "\r\n\r\n";

    write(client_socket, buffer(header));
    write(client_socket, buffer(body));

    boost::beast::flat_buffer b;
    auto transform = [](auto&) {};

    auto work_guard = boost::asio::make_work_guard(ioc.get_executor());
    auto thread = std::thread([&ioc] { ioc.run(); });

    parser<true, double_buffer_body> p;
    serializer<true, double_buffer_body, fields> sr{p.get()};

    co_spawn(
        ioc,
        [&]() -> coro<void> {
            co_await async_read_header(server_socket, b, p);
            transform(p.get());
            co_await async_write_header(server_socket, sr);
        },
        boost::asio::use_future)
        .get();

    co_spawn(ioc,
             async_relay_body<2_KiB>(server_socket, server_socket, b, p, sr),
             boost::asio::use_future)
        .get();

    work_guard.reset();
    thread.join();

    boost::system::error_code ec;
    std::vector<char> recv_buf(16 * 1024);
    size_t n = client_socket.read_some(buffer(recv_buf), ec);
    std::string output_str(recv_buf.data(), n);

    BOOST_CHECK_NE(output_str.find("POST /upload HTTP/1.1"), std::string::npos);
    BOOST_CHECK_NE(output_str.find("Host: example.com"), std::string::npos);
    BOOST_CHECK_NE(output_str.find("User-Agent: test"), std::string::npos);

    auto body_pos = output_str.find("\r\n\r\n");
    BOOST_REQUIRE(body_pos != std::string::npos);
    body_pos += 4;
    std::string_view received_body(&recv_buf[body_pos], n - body_pos);
    BOOST_TEST(received_body == std::string_view(body.data(), body.size()));
}

} // namespace uh::cluster::proxy::cache::disk
