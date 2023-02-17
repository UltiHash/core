#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "uhLibOptions Logging Options Test"
#endif

#include <boost/test/unit_test.hpp>
#include <options/loader.h>

#include <string>
#include <vector>


using namespace uh::options;

namespace po = boost::program_options;

namespace
{

// ---------------------------------------------------------------------

struct test_opts : public options
{
    test_opts() : options("test")
    {
        visible().add_options()
            ("test-string,S", po::value<std::string>(&string));
        hidden().add_options()
            ("positional", po::value<std::vector<std::string>>(&positionals)->multitoken());
        file().add_options()
            ("test.test-string", po::value<std::string>(&string))
            ("global-string", po::value<std::string>(&global));

        positional_mapping("positional", -1);
    }

    std::string string = "default value";
    std::string global = "global default value";
    std::vector<std::string> positionals;
};

// ---------------------------------------------------------------------

struct fixture
{
    fixture()
    {
        loader.add(options);
    }

    test_opts options;
    uh::options::loader loader;
};

// ---------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE(loader_command_line, fixture)
{
    const char* args[] = { "test-program",
                           "--test-string", "lorem ipsum",
                           "pos1", "pos2", "pos3"
                         };

    loader.parse(sizeof(args) / sizeof(char*), args);
    BOOST_CHECK_EQUAL(options.string, "lorem ipsum");

    BOOST_CHECK_EQUAL(options.positionals.size(), 3u);
    BOOST_CHECK_EQUAL(*options.positionals.begin(), "pos1");
    BOOST_CHECK_EQUAL(*(options.positionals.begin() + 1), "pos2");
    BOOST_CHECK_EQUAL(*(options.positionals.begin() + 2), "pos3");
}

// ---------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE(loader_cli_failure, fixture)
{
    const char* args[] = { "test-program",
                           "--unsupported-value", "lorem ipsum",
                         };

    BOOST_CHECK_THROW(loader.parse(sizeof(args) / sizeof(char*), args), std::exception);
    BOOST_CHECK_EQUAL(options.string, "default value");
}

// ---------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE(loader_config_file, fixture)
{
    const char* config_file =
        "global-string = global ipsum\n"
        "[test]\n"
        "test-string = lorem ipsum\n";

    std::stringstream cfg(config_file);
    loader.parse(cfg);

    BOOST_CHECK_EQUAL(options.string, "lorem ipsum");
    BOOST_CHECK_EQUAL(options.global, "global ipsum");
}

// ---------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE(mixed_config_first, fixture)
{
    const char* config_file =
        "[test]\n"
        "test-string = config file wins\n";

    const char* args[] = { "test-program",
                           "--test-string", "cli wins",
                         };

    std::stringstream cfg(config_file);
    loader.parse(cfg);
    auto act = loader.parse(sizeof(args) / sizeof(char*), args);

    BOOST_CHECK_EQUAL(act, action::proceed);
    BOOST_CHECK_EQUAL(options.string, "cli wins");
}

// ---------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE(mixed_cli_first, fixture)
{
    const char* config_file =
        "[test]\n"
        "test-string = config file wins\n";

    const char* args[] = { "test-program",
                           "--test-string", "cli wins",
                         };

    std::stringstream cfg(config_file);
    auto act = loader.parse(sizeof(args) / sizeof(char*), args);
    loader.parse(cfg);

    BOOST_CHECK_EQUAL(act, action::proceed);
    BOOST_CHECK_EQUAL(options.string, "config file wins");
}

// ---------------------------------------------------------------------

} // namespace
