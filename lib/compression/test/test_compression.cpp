#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "uhLibComp compression tests"
#endif

#include <boost/test/unit_test.hpp>
#include <boost/test/data/test_case.hpp>
#include <boost/mpl/vector.hpp>

#include <io/sstream_device.h>
#include <compression/compression.h>
#include <compression/none.h>


using namespace uh;
using namespace uh::io;

namespace
{

// ---------------------------------------------------------------------

const static std::string TEST_TEXT =
    "Lorem ipsum dolor sit amet, consectetur adipiscing "
    "elit, sed do eiusmod tempor incididunt ut labore et "
    "dolore magna aliqua. Ut enim ad minim veniam, quis "
    "nostrud exercitation ullamco laboris nisi ut "
    "aliquip ex ea commodo consequat. Duis aute irure "
    "dolor in reprehenderit in voluptate velit esse "
    "cillum dolore eu fugiat nulla pariatur. Excepteur "
    "sint occaecat cupidatat non proident, sunt in culpa "
    "qui officia deserunt mollit anim id est laborum.";

// ---------------------------------------------------------------------

typedef boost::mpl::vector<
    comp::none
> comp_types;

// ---------------------------------------------------------------------

static const std::list<comp::type> comp_enum_types = {
    comp::type::none
};

// ---------------------------------------------------------------------

struct Fixture {};

// ---------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE_TEMPLATE( rw_invariant, T, comp_types, Fixture )
{
    io::sstream_device base;

    auto comp_dev = T(base);
    comp_dev.write(TEST_TEXT);

    std::vector<char> buffer(TEST_TEXT.size());
    comp_dev.read(buffer);

    BOOST_CHECK_EQUAL(std::string(&buffer[0], buffer.size()), TEST_TEXT);
}

// ---------------------------------------------------------------------

BOOST_DATA_TEST_CASE( rw_invariant_by_type, comp_enum_types )
{
    io::sstream_device base;

    auto comp_dev = comp::create(base, sample);
    comp_dev->write(TEST_TEXT);

    std::vector<char> buffer(TEST_TEXT.size());
    comp_dev->read(buffer);

    BOOST_CHECK_EQUAL(std::string(&buffer[0], buffer.size()), TEST_TEXT);
}

// ---------------------------------------------------------------------

} // namespace


