#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "uhLibOptions Logging Options Test"
#endif

#include <boost/test/unit_test.hpp>
#include <options/logging_options.h>


using namespace boost::log;
using namespace uh::log;
using namespace uh::options;

namespace
{

// ---------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( stderr_sink )
{
    const char* args[] = { "test-program",
                           "--log-stderr", "DEBUG" };

    logging_options opts;
    opts.apply(opts);
    opts.parse(sizeof(args) / sizeof(char*), args);
    auto cfg = opts.config();

    BOOST_CHECK_EQUAL(cfg.sinks.size(), 1);
    BOOST_CHECK_EQUAL(
        cfg.sinks.front(),
        (sink_config{ .type = sink_type::cerr, .level = trivial::debug }));
}

// ---------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( stderr_sink_short )
{
    const char* args[] = { "test-program",
                           "-E", "DEBUG" };

    logging_options opts;
    opts.apply(opts);
    opts.parse(sizeof(args) / sizeof(char*), args);
    auto cfg = opts.config();

    BOOST_CHECK_EQUAL(cfg.sinks.size(), 1);
    BOOST_CHECK_EQUAL(
        cfg.sinks.front(),
        (sink_config{ .type = sink_type::cerr, .level = trivial::debug }));
}

// ---------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( stdout_sink )
{
    const char* args[] = { "test-program",
                           "--log-stdout", "DEBUG" };

    logging_options opts;
    opts.apply(opts);
    opts.parse(sizeof(args) / sizeof(char*), args);
    auto cfg = opts.config();

    BOOST_CHECK_EQUAL(cfg.sinks.size(), 1);
    BOOST_CHECK_EQUAL(
        cfg.sinks.front(),
        (sink_config{ .type = sink_type::cout, .level = trivial::debug }));
}

// ---------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( stdout_sink_short )
{
    const char* args[] = { "test-program",
                           "-O", "DEBUG" };

    logging_options opts;
    opts.apply(opts);
    opts.parse(sizeof(args) / sizeof(char*), args);
    auto cfg = opts.config();

    BOOST_CHECK_EQUAL(cfg.sinks.size(), 1);
    BOOST_CHECK_EQUAL(
        cfg.sinks.front(),
        (sink_config{ .type = sink_type::cout, .level = trivial::debug }));
}

// ---------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( multiple_file_sinks )
{
    const char* args[] = { "test-program",
                           "--log-file", "/var/log/uh/debug.log:DEBUG",
                           "--log-file", "/var/log/uh/info.log" };

    logging_options opts;
    opts.apply(opts);
    opts.parse(sizeof(args) / sizeof(char*), args);
    auto cfg = opts.config();

    BOOST_CHECK_EQUAL(cfg.sinks.size(), 2);

    std::list<sink_config> sinks{
        { .type = sink_type::file, .filename = "/var/log/uh/debug.log", .level = trivial::debug },
        { .type = sink_type::file, .filename = "/var/log/uh/info.log", .level = trivial::info }
    };

    BOOST_CHECK_EQUAL_COLLECTIONS(cfg.sinks.begin(), cfg.sinks.end(),
                                  sinks.begin(), sinks.end());
}

// ---------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( multiple_file_sinks_short )
{
    const char* args[] = { "test-program",
                           "-F", "/var/log/uh/debug.log:DEBUG",
                           "-F", "/var/log/uh/info.log" };

    logging_options opts;
    opts.apply(opts);
    opts.parse(sizeof(args) / sizeof(char*), args);
    auto cfg = opts.config();

    BOOST_CHECK_EQUAL(cfg.sinks.size(), 2);

    std::list<sink_config> sinks{
        { .type = sink_type::file, .filename = "/var/log/uh/debug.log", .level = trivial::debug },
        { .type = sink_type::file, .filename = "/var/log/uh/info.log", .level = trivial::info }
    };

    BOOST_CHECK_EQUAL_COLLECTIONS(cfg.sinks.begin(), cfg.sinks.end(),
                                  sinks.begin(), sinks.end());
}

// ---------------------------------------------------------------------

}

