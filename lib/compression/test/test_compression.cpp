#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "uhLibComp compression tests"
#endif

#include <boost/test/unit_test.hpp>
#include <boost/test/data/test_case.hpp>
#include <boost/mpl/vector.hpp>

#include <test/ipsum.h>
#include <io/buffer.h>
#include <compression/compression.h>
#include <compression/none.h>
#include <compression/brotli.h>


using namespace uh;
using namespace uh::io;
using namespace uh::test;

namespace
{

// ---------------------------------------------------------------------

typedef boost::mpl::vector<
    comp::none,
    comp::brotli
> comp_types;

// ---------------------------------------------------------------------

static const std::list<comp::type> comp_enum_types = {
    comp::type::none,
    comp::type::brotli
};

// ---------------------------------------------------------------------

struct Fixture {};

// ---------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE_TEMPLATE( rw_invariant, T, comp_types, Fixture )
{
    io::buffer base;

    {
        auto comp_dev = T(base);
        comp_dev.write(LOREM_IPSUM);
    }

    std::vector<char> buffer(LOREM_IPSUM.size());
    auto comp_dev = T(base);
    comp_dev.read(buffer);

    BOOST_CHECK_EQUAL(std::string(&buffer[0], buffer.size()), LOREM_IPSUM);
}

// ---------------------------------------------------------------------

BOOST_DATA_TEST_CASE( rw_invariant_by_type, comp_enum_types )
{
    io::buffer base;

    {
        auto comp_dev = comp::create(base, sample);
        comp_dev->write(LOREM_IPSUM);
    }

    std::vector<char> buffer(LOREM_IPSUM.size());
    auto comp_dev = comp::create(base, sample);
    comp_dev->read(buffer);

    BOOST_CHECK_EQUAL(std::string(&buffer[0], buffer.size()), LOREM_IPSUM);
}

// ---------------------------------------------------------------------

} // namespace


