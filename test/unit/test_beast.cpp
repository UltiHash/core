
#define BOOST_TEST_MODULE "service tests"
#include <boost/test/unit_test.hpp>

#include "test_config.h"

#include <boost/asio/buffer.hpp>
#include <boost/beast/http/parser.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/optional/optional_io.hpp>
#include <iostream>
#include <string>

using namespace std;
using namespace boost::beast;

// ------------- Tests Suites Follow --------------

namespace uh::cluster {

BOOST_AUTO_TEST_SUITE(a_beast_parser)

BOOST_AUTO_TEST_CASE(parses_header_and_body) {
    std::string header_and_body =
        "POST /112 HTTP/1.1\r\n"
        "Host: 127.0.0.1:8080\r\n"
        "User-Agent: curl/7.68.0\r\n"
        "Accept: */*\r\nContent-Length: 13\r\n"
        "Content-Type: application/x-www-form-urlencoded\r\n"
        "\r\n"
        "1212312312312";

    http::request_parser<http::string_body> parser;
    boost::beast::error_code ec;

    parser.eager(true);
    parser.put(
        boost::asio::buffer(header_and_body.data(), header_and_body.size()),
        ec);
    BOOST_TEST(!ec);

    parser.put_eof(ec);
    BOOST_TEST(!ec);

    BOOST_TEST(parser.is_header_done());
}

BOOST_AUTO_TEST_CASE(parses_header_and_body_seperately) {
    std::string header = "POST /112 HTTP/1.1\r\n"
                         "Host: 127.0.0.1:8080\r\n"
                         "User-Agent: curl/7.68.0\r\n"
                         "Accept: */*\r\nContent-Length: 13\r\n"
                         "Content-Type: application/x-www-form-urlencoded\r\n"
                         "\r\n";
    std::string body = "1212312312312";

    http::request_parser<http::string_body> parser;
    boost::beast::error_code ec;

    // parse header
    parser.put(boost::asio::buffer(header.data(), header.size()), ec);
    BOOST_TEST(!ec);

    parser.put(boost::asio::buffer(body.data(), body.size()), ec);
    BOOST_TEST(!ec);

    parser.put_eof(ec);
    BOOST_TEST(!ec);

    BOOST_TEST(parser.is_header_done());
}

BOOST_AUTO_TEST_CASE(parses_s3_header) {
    // clang-format off
    auto header = std::string(
        "PUT /xxx/classical.00051.wav HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "Accept-Encoding: identity\r\n"
        "User-Agent: Boto3/1.34.46 md/Botocore#1.34.46 ua/2.0 "
            "os/linux#6.14.0-27-generic md/arch#x86_64 lang/python#3.12.3 "
            "md/pyimpl#CPython cfg/retry-mode#standard Botocore/1.34.46\r\n"
        "Content-MD5: 8ZIZc/SNPVXoAxpcGBGiIA==\r\n"
        "Expect: 100-continue\r\n"
        "X-Amz-Date: 20250806T084505Z\r\n"
        "X-Amz-Content-SHA256: "
            "529f84176f8e05feb59b151db3a07b3347c6cd9fa945b77c3f7b1da18a409ac0\r\n"
        "Authorization: AWS4-HMAC-SHA256 "
            "Credential=FANCY-ROOT-KEY/20250806/eu-central-1/s3/aws4_request, "
            "SignedHeaders=content-md5;host;x-amz-content-sha256;x-amz-date, "
            "Signature="
            "a5f7d5622c04829eff143c51f629c07c1d202a34c5d4b61cb56696ac3405d6a1\r\n"
        "amz-sdk-invocation-id: b0cf8641-8ca1-4d92-b4a4-29cf2f1ada10\r\n"
        "amz-sdk-request: attempt=1\r\n"
        "Content-Length: 1322732\r\n"
        "\r\n");
    // clang-format on

    http::request_parser<http::string_body> parser;
    parser.body_limit((std::numeric_limits<std::uint64_t>::max)());
    boost::beast::error_code ec;

    // parse header
    parser.put(boost::asio::buffer(header.data(), header.size()), ec);
    std::cout << ec.message() << std::endl;
    BOOST_TEST(!ec);

    BOOST_TEST(parser.is_header_done());

    auto headers = parser.release();
    BOOST_TEST(headers["Content-Length"] == "1322732");
}

BOOST_AUTO_TEST_CASE(works_well_with_tcp_communication) {
    boost::asio::io_context io;
    boost::asio::ip::tcp::acceptor acceptor(io,
                                            {boost::asio::ip::tcp::v4(), 0});
    auto endpoint = acceptor.local_endpoint();

    boost::asio::ip::tcp::socket server_sock(io);
    boost::asio::ip::tcp::socket client_sock(io);

    client_sock.connect(endpoint);
    acceptor.accept(server_sock);

    // Client writes
    std::string req =
        "GET / HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n";
    boost::asio::write(client_sock, boost::asio::buffer(req));

    // Server reads
    boost::asio::streambuf buffer;
    boost::beast::http::request_parser<boost::beast::http::empty_body> parser;
    boost::beast::error_code ec;
    boost::asio::read_until(server_sock, buffer, "\r\n\r\n");

    // Server gets the header from the buffer
    std::istream is(&buffer);
    std::string header;
    header.resize(buffer.size());
    is.read(&header[0], buffer.size());

    // Server parses the header
    parser.put(boost::asio::buffer(header.data(), header.size()), ec);
    BOOST_TEST(!ec);
    BOOST_TEST(parser.is_header_done());

    // Server writes a response
    boost::beast::http::response<boost::beast::http::string_body> res{
        boost::beast::http::status::ok, parser.get().version()};
    res.set(boost::beast::http::field::server, "TestServer");
    res.body() = "Hello, HTTP!";
    res.prepare_payload();
    boost::beast::http::write(server_sock, res);

    // Client reads the response
    boost::asio::streambuf resp_buffer;
    boost::beast::http::response_parser<boost::beast::http::string_body>
        resp_parser;
    boost::asio::read_until(client_sock, resp_buffer, "\r\n\r\n");
    std::istream resp_is(&resp_buffer);
    std::string resp_header;
    resp_header.resize(resp_buffer.size());
    resp_is.read(&resp_header[0], resp_buffer.size());

    // Client parses the response header
    resp_parser.put(boost::asio::buffer(resp_header.data(), resp_header.size()),
                    ec);
    BOOST_TEST(!ec);
    BOOST_TEST(resp_parser.is_header_done());
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(a_beast_flat_buffer)

BOOST_AUTO_TEST_CASE(transfers_output_buffer_to_input_when_committing) {
    boost::beast::flat_buffer buffer;
    auto writer = buffer.prepare(20);
    auto data = std::string("Hello, World!");
    std::memcpy(static_cast<void*>(writer.data()), data.data(), data.size());

    buffer.commit(data.size());

    auto reader = buffer.data();
    BOOST_TEST(reader.size() == data.size());
    BOOST_TEST(std::string(static_cast<char*>(reader.data()), reader.size()) ==
               data);
}

BOOST_AUTO_TEST_CASE(doesnt_update_input_buffer_without_committing) {
    boost::beast::flat_buffer buffer;
    auto writer = buffer.prepare(20);
    auto data = std::string("Hello, World!");
    std::memcpy(static_cast<void*>(writer.data()), data.data(), data.size());

    auto reader = buffer.data();
    BOOST_TEST(reader.size() == 0);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(beast)

BOOST_AUTO_TEST_CASE(reads_and_parses_header_only) {
    boost::asio::io_context io;
    boost::asio::ip::tcp::acceptor acceptor(io,
                                            {boost::asio::ip::tcp::v4(), 0});
    auto endpoint = acceptor.local_endpoint();

    boost::asio::ip::tcp::socket server_sock(io);
    boost::asio::ip::tcp::socket client_sock(io);

    client_sock.connect(endpoint);
    acceptor.accept(server_sock);

    std::string req = "POST /test HTTP/1.1\r\nHost: "
                      "localhost\r\nContent-Length: 5\r\n\r\nabcde";
    boost::asio::write(client_sock, boost::asio::buffer(req));

    boost::beast::flat_buffer buffer;
    boost::beast::http::request_parser<boost::beast::http::empty_body> parser;

    boost::beast::http::read_header(server_sock, buffer, parser);

    BOOST_TEST(parser.is_header_done());
    BOOST_TEST(parser.get().method_string() == "POST");
    BOOST_TEST(parser.get().target() == "/test");
    BOOST_TEST(parser.get()[boost::beast::http::field::host] == "localhost");
}

BOOST_AUTO_TEST_CASE(parses_header_which_is_read_using_read_until) {
    boost::asio::io_context io;
    boost::asio::ip::tcp::acceptor acceptor(io,
                                            {boost::asio::ip::tcp::v4(), 0});
    auto endpoint = acceptor.local_endpoint();

    boost::asio::ip::tcp::socket server_sock(io);
    boost::asio::ip::tcp::socket client_sock(io);

    client_sock.connect(endpoint);
    acceptor.accept(server_sock);

    std::string req = "POST /test HTTP/1.1\r\nHost: "
                      "localhost\r\nContent-Length: 5\r\n\r\nabcde";
    boost::asio::write(client_sock, boost::asio::buffer(req));

    boost::asio::streambuf resp_buffer;
    boost::beast::http::request_parser<boost::beast::http::string_body>
        resp_parser;
    boost::asio::read_until(server_sock, resp_buffer, "\r\n\r\n");
    std::istream resp_is(&resp_buffer);
    std::string resp_header;
    resp_header.resize(resp_buffer.size());
    resp_is.read(&resp_header[0], resp_buffer.size());

    // Client parses the request header
    boost::beast::error_code ec;
    resp_parser.put(boost::asio::buffer(resp_header.data(), resp_header.size()),
                    ec);

    BOOST_TEST(resp_parser.is_header_done());
    BOOST_TEST(resp_parser.get().method_string() == "POST");
    BOOST_TEST(resp_parser.get().target() == "/test");
    BOOST_TEST(resp_parser.get()[boost::beast::http::field::host] ==
               "localhost");
}

BOOST_AUTO_TEST_CASE(supports_vector_of_const_buffers) {
    boost::asio::io_context io;
    boost::asio::ip::tcp::acceptor acceptor(io,
                                            {boost::asio::ip::tcp::v4(), 0});
    auto endpoint = acceptor.local_endpoint();

    boost::asio::ip::tcp::socket server_sock(io);
    boost::asio::ip::tcp::socket client_sock(io);

    client_sock.connect(endpoint);
    acceptor.accept(server_sock);

    std::string req_1 = "POST /test HTTP/1.1\r\n";
    std::string req_2 = "Host: localhost\r\nContent-Length: 5\r\n\r\nabcde";

    std::vector<boost::asio::const_buffer> req = {boost::asio::buffer(req_1),
                                                  boost::asio::buffer(req_2)};
    boost::asio::write(client_sock, req);
    std::cout << "Request sent: " << req_1 << req_2 << std::endl;

    boost::asio::streambuf resp_buffer;
    boost::beast::http::request_parser<boost::beast::http::string_body>
        resp_parser;
    std::cout << "Read started..." << std::endl;
    boost::asio::read_until(server_sock, resp_buffer, "\r\n\r\n");
    std::cout << "Read done" << std::endl;
    std::istream resp_is(&resp_buffer);
    std::string resp_header;
    resp_header.resize(resp_buffer.size());
    resp_is.read(&resp_header[0], resp_buffer.size());

    // Client parses the request header
    boost::beast::error_code ec;
    resp_parser.put(boost::asio::buffer(resp_header.data(), resp_header.size()),
                    ec);

    BOOST_TEST(resp_parser.is_header_done());
    BOOST_TEST(resp_parser.get().method_string() == "POST");
    BOOST_TEST(resp_parser.get().target() == "/test");
    BOOST_TEST(resp_parser.get()[boost::beast::http::field::host] ==
               "localhost");
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace uh::cluster
