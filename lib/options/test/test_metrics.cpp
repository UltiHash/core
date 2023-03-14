#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "uhLibOptions Logging Options Test"
#endif

#include <boost/test/unit_test.hpp>
#include <options/metrics_options.h>
#include <options/loader.h>


using namespace uh::options;

namespace
{

// ---------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(defaults)
{
    metrics_options opts;

    const auto& cfg = opts.config();

    BOOST_CHECK_EQUAL(cfg.address, "0.0.0.0:8080");
    BOOST_CHECK_EQUAL(cfg.threads, 2u);
    BOOST_CHECK_EQUAL(cfg.path, "/metrics");
}

// ---------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(parse)
{
    const char* args[] = { "test-program",
                           "--metrics-address", "1.2.3.4:1234",
                           "--metrics-threads", "5",
                           "--metrics-path", "/foo" };

    metrics_options opts;
    loader().add(opts).evaluate(sizeof(args) / sizeof(char*), args).finalize();
    const auto& cfg = opts.config();

    BOOST_CHECK_EQUAL(cfg.address, "1.2.3.4:1234");
    BOOST_CHECK_EQUAL(cfg.threads, 5u);
    BOOST_CHECK_EQUAL(cfg.path, "/foo");
}

// ---------------------------------------------------------------------

} // namespace
