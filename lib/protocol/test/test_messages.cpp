#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "uhLibProtocol Message Tests"
#endif

#include <boost/test/unit_test.hpp>
#include <protocol/messages.h>
#include <sstream>


using namespace uh::protocol;

namespace
{

// ---------------------------------------------------------------------

blob to_blob(const std::string& s)
{
    return blob(s.data(), s.data() + s.size());
}

// ---------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( status_message )
{
    {
        std::stringstream s;
        write(s, status{ .code = status::OK, .message = "will not be serialized" });

        check_status(s);
        BOOST_TEST(true);
    }
    {
        std::stringstream s;
        write(s, status{ .code = status::FAILED, .message = "error message" });

        BOOST_CHECK_EXCEPTION(check_status(s), std::exception,
            [](const auto& e) { return std::string(e.what()).find("error message") != std::string::npos; });
    }
}

// ---------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( hello_request )
{
    std::stringstream s;
    write(s, hello::request{ .client_version = "0.0.1" });

    auto ch = s.get();
    BOOST_TEST(ch == hello::request_id);

    hello::request req;
    read(s, req);
    BOOST_TEST(req.client_version == "0.0.1");
}

// ---------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( hello_response )
{
    std::stringstream s;
    write(s, status{ .code = status::OK });
    write(s, hello::response{ .server_version = "1.0.0",
                              .protocol_version = 0x55 });

    hello::response res;
    read(s, res);
    BOOST_TEST(res.server_version == "1.0.0");
    BOOST_TEST(res.protocol_version == 0x55);
}

// ---------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( read_block_request )
{
    std::stringstream s;
    write(s, read_block::request{ .hash = to_blob("hashed data") });

    auto ch = s.get();
    BOOST_TEST(ch == read_block::request_id);

    read_block::request req;
    read(s, req);
    BOOST_TEST(req.hash == to_blob("hashed data"));
}

// ---------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( read_block_response )
{
    std::stringstream s;
    write(s, status{ .code = status::OK });
    write(s, read_block::response{});

    read_block::response res;
    read(s, res);
}

// ---------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( quit_request )
{
    std::stringstream s;
    write(s, quit::request{ .reason = "bye" });

    auto ch = s.get();
    BOOST_TEST(ch == quit::request_id);

    quit::request req;
    read(s, req);
    BOOST_TEST(req.reason == "bye");
}

// ---------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( quit_response )
{
    std::stringstream s;
    write(s, status{ .code = status::OK });
    write(s, quit::response{});

    quit::response res;
    read(s, res);
}

// ---------------------------------------------------------------------

} // namespace
