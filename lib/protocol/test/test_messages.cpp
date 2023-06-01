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
        ser.sync();

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
    ser.sync();
    char ch = ser.read<char>();
    BOOST_TEST(ch == hello::request_id);

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
            .server_uuid = "01234567-89AB-CDEF-0123-456789ABCDEF"
            .protocol_version = 0x55 });
    ser.sync();

    hello::response res;
    read(ser, res);
    BOOST_TEST(res.server_version == "1.0.0");
    BOOST_TEST(res.server_uid == "01234567-89AB-CDEF-0123-456789ABCDEF");
    BOOST_TEST(res.protocol_version == 0x55);
}

// ---------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( quit_request )
{
    uh::io::sstream_device dev;
    uh::serialization::buffered_serialization ser (dev);
    write(ser, quit::request{ .reason = "bye" });
    ser.sync();

    char ch = ser.read<char>();
    BOOST_TEST(ch == quit::request_id);

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
    ser.sync();

    quit::response res;
    read(ser, res);
}

// ---------------------------------------------------------------------

} // namespace
