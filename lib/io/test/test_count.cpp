#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "uhLibIo Count Tests"
#endif

#include <test/ipsum.h>
#include <io/buffer.h>
#include <io/count.h>

#include <boost/test/unit_test.hpp>


using namespace uh;
using namespace uh::io;
using namespace uh::test;

namespace
{

// ---------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( write )
{
    buffer b;
    count c(b);

    c.write(LOREM_IPSUM);
    c.write(LOREM_IPSUM);

    BOOST_CHECK_EQUAL(c.written(), 2 * LOREM_IPSUM.size());
    BOOST_CHECK_EQUAL(c.read(), 0);
}

// ---------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( read )
{
    buffer b;
    count c(b);

    b.write(LOREM_IPSUM);
    b.write(LOREM_IPSUM);

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

