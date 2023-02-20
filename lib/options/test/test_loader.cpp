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

        positional_mapping("positional", -1);
    }

    std::string string = "default value";
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

    loader.evaluate(sizeof(args) / sizeof(char*), args);
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

    BOOST_CHECK_THROW(loader.evaluate(sizeof(args) / sizeof(char*), args), std::exception);
    BOOST_CHECK_EQUAL(options.string, "default value");
}

// ---------------------------------------------------------------------

} // namespace
