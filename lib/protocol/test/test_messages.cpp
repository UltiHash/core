#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "uhLibProtocol Message Tests"
#endif

#include <boost/test/unit_test.hpp>
#include <protocol/messages.h>
#include <sstream>
#include "io/sstream_device.h"


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
        uh::io::sstream_device dev;
        uh::serialization::buffered_serialization ser (dev);
        write(ser, status{ .code = status::OK, .message = "will not be serialized" });

        ser.sync();
        check_status(ser);
        BOOST_TEST(true);
    }
    {
        uh::io::sstream_device dev;
        uh::serialization::buffered_serialization ser (dev);
        write(ser, status{ .code = status::FAILED, .message = "error message" });

        BOOST_CHECK_EXCEPTION(check_status(ser), std::exception,
            [](const auto& e) { return std::string(e.what()).find("error message") != std::string::npos; });
    }
}

// ---------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( hello_request )
{
    uh::io::sstream_device dev;
    uh::serialization::buffered_serialization ser (dev);
    write(ser, hello::request{ .client_version = "0.0.1" });

    char ch[1];
    dev.read(ch);
    BOOST_TEST(ch[0] == hello::request_id);

    hello::request req;
    read(ser, req);
    BOOST_TEST(req.client_version == "0.0.1");
}

// ---------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( hello_response )
{
    uh::io::sstream_device dev;
    uh::serialization::buffered_serialization ser (dev);
    write(ser, status{ .code = status::OK });
    write(ser, hello::response{ .server_version = "1.0.0",
                              .protocol_version = 0x55 });

    hello::response res;
    read(ser, res);
    BOOST_TEST(res.server_version == "1.0.0");
    BOOST_TEST(res.protocol_version == 0x55);
}

// ---------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( write_block_request )
{
    uh::io::sstream_device dev;
    uh::serialization::buffered_serialization ser (dev);
    write(ser, write_block::request{ .content = to_blob("lorem ipsum") });

    char ch[1];
    dev.read(ch);
    BOOST_TEST(ch[0] == write_block::request_id);

    write_block::request req;
    read(ser, req);
    BOOST_TEST(req.content == to_blob("lorem ipsum"));
}

// ---------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( write_block_response )
{
    uh::io::sstream_device dev;
    uh::serialization::buffered_serialization ser (dev);
    write(ser, status{ .code = status::OK });
    write(ser, write_block::response{ .hash = to_blob("hashed data"), .effective_size = 11});

    write_block::response res;
    read(ser, res);
    BOOST_TEST(res.hash == to_blob("hashed data"));
    BOOST_TEST(res.effective_size == 11);
}

// ---------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( read_block_request )
{
    uh::io::sstream_device dev;
    uh::serialization::buffered_serialization ser (dev);
    write(ser, read_block::request{ .hash = to_blob("hashed data") });

    char ch[1];
    dev.read(ch);
    BOOST_TEST(ch[0] == read_block::request_id);

    read_block::request req;
    read(ser, req);
    BOOST_TEST(req.hash == to_blob("hashed data"));
}

// ---------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( read_block_response )
{
    uh::io::sstream_device dev;
    uh::serialization::buffered_serialization ser (dev);
    write(ser, status{ .code = status::OK });
    write(ser, read_block::response{});

    read_block::response res;
    read(ser, res);
}

// ---------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( quit_request )
{
    uh::io::sstream_device dev;
    uh::serialization::buffered_serialization ser (dev);
    write(ser, quit::request{ .reason = "bye" });

    char ch[1];
    dev.read(ch);
    BOOST_TEST(ch[0] == quit::request_id);

    quit::request req;
    read(ser, req);
    BOOST_TEST(req.reason == "bye");
}

// ---------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( quit_response )
{
    uh::io::sstream_device dev;
    uh::serialization::buffered_serialization ser (dev);
    write(ser, status{ .code = status::OK });
    write(ser, quit::response{});

    quit::response res;
    read(ser, res);
}

// ---------------------------------------------------------------------

} // namespace
