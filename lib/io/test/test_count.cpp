#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "uhLibIo Count Tests"
#endif

#include <boost/test/unit_test.hpp>

#include <io/buffer.h>
#include <io/count.h>


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

BOOST_AUTO_TEST_CASE( write )
{
    buffer b;
    count c(b);

    c.write(TEST_TEXT);
    c.write(TEST_TEXT);

    BOOST_CHECK_EQUAL(c.written(), 2 * TEST_TEXT.size());
    BOOST_CHECK_EQUAL(c.read(), 0);
}

// ---------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( read )
{
    buffer b;
    count c(b);

    b.write(TEST_TEXT);
    b.write(TEST_TEXT);

    std::vector<char> rb(12);
    for (auto i = 0u; i < 4; ++i)
    {
        c.read(rb);
    }

    BOOST_CHECK_EQUAL(c.written(), 0);
    BOOST_CHECK_EQUAL(c.read(), 4 * rb.size());
}

// ---------------------------------------------------------------------

} // namespace

