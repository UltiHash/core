#define BOOST_TEST_MODULE "disk-cache's http body tests"

#include <boost/test/unit_test.hpp>

#include <util/dedupe_fixture.h>

#include <proxy/cache/disk/body.h>

#include <common/utils/random.h>
#include <entrypoint/http/stream.h>
#include <proxy/cache/disk/body.h>

#include <boost/beast/http/parser.hpp>
#include <boost/beast/http/string_body.hpp>

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
    reader_body body(data_view);
    boost::asio::co_spawn(m_ioc, async_read(stream, body, content_length),
                          boost::asio::use_future)
        .get();

    // 7. Verify body was stored and can be read back
    auto objh = body.get_object_handle();
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
    double_buffered_writer_body body(data_view, std::move(objh));

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

    // Client writes body using async_write and writer_body
    boost::asio::co_spawn(m_ioc, async_write(stream, body),
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

BOOST_AUTO_TEST_CASE(supports_write_using_smaller_buffer) {}

BOOST_AUTO_TEST_SUITE_END()

} // namespace uh::cluster::proxy::cache::disk
