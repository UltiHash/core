#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "uhLibIo GroupGenerator Tests"
#endif

#include <test/ipsum.h>
#include <io/buffer.h>
#include <io/group_generator.h>
#include <io/test_generator.h>

#include <boost/test/unit_test.hpp>


using namespace uh::io;
using namespace uh::io::test;
using namespace uh::test;

namespace
{

// ---------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(group_gen)
{
    group_generator gg;

    gg.append(std::make_unique<generator>( std::span<const char>{ LOREM_IPSUM.begin(), 30 } ));
    gg.append(std::make_unique<generator>( std::span<const char>{ LOREM_IPSUM.begin() + 30, LOREM_IPSUM.end()} ));

    buffer b;
    auto count = b.write_range(gg);

    BOOST_CHECK_EQUAL(LOREM_IPSUM, std::string(b.data().begin(), b.data().begin() + count));
}

// ---------------------------------------------------------------------

} // namespace
