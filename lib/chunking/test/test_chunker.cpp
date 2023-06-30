#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "uhLibChunk Chunker Tests"
#endif

#include <boost/test/unit_test.hpp>
#include <boost/mpl/vector.hpp>

#include <chunking/fast_cdc.h>
#include <chunking/fixed_size_chunker.h>
#include <chunking/gear.h>
#include <chunking/mod_chunker.h>
#include <chunking/rabin_fp.h>
#include <util/random.h>

#include <numeric>


using namespace uh;
using namespace uh::chunking;

namespace
{

// ---------------------------------------------------------------------

typedef boost::mpl::vector<
    fixed_size_chunker,
    rabin_fp,
    mod_chunker,
    fast_cdc,
    gear
> chunker_types;

// ---------------------------------------------------------------------

struct Fixture {};

// ---------------------------------------------------------------------

/**
 * To be implemented for each type in `chunker_types`
 */
template <typename T>
std::unique_ptr<T> make_test_chunker();

// ---------------------------------------------------------------------

template <>
std::unique_ptr<fixed_size_chunker> make_test_chunker<fixed_size_chunker>()
{
    return std::make_unique<fixed_size_chunker>(512u);
}

// ---------------------------------------------------------------------

template <>
std::unique_ptr<fast_cdc> make_test_chunker<fast_cdc>()
{
    return std::make_unique<fast_cdc>(fast_cdc_config());
}

// ---------------------------------------------------------------------

template <>
std::unique_ptr<gear> make_test_chunker<gear>()
{
    return std::make_unique<gear>(gear_config());
}

// ---------------------------------------------------------------------

template <>
std::unique_ptr<mod_chunker> make_test_chunker<mod_chunker>()
{
    return std::make_unique<mod_chunker>(mod_cdc_config());
}

// ---------------------------------------------------------------------

template <>
std::unique_ptr<rabin_fp> make_test_chunker<rabin_fp>()
{
    return std::make_unique<rabin_fp>();
}

// ---------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE_TEMPLATE( empty_buffer, T, chunker_types, Fixture )
{
    auto chunker = make_test_chunker<T>();

    std::vector<char> empty;

    auto chunks = chunker->chunk(empty);
    BOOST_CHECK(chunks.empty());
}

// ---------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE_TEMPLATE( buffer_size_sum, T, chunker_types, Fixture )
{
    auto chunker = make_test_chunker<T>();

    const std::size_t length = 8 * 1024 * 1024;

    std::vector<char> data(length);
    std::generate(data.begin(), data.end(), util::random_generator<char>());

    auto chunks = chunker->chunk(data);
    BOOST_CHECK(!chunks.empty());

    BOOST_REQUIRE_EQUAL(std::accumulate(chunks.begin(), chunks.end(), 0u), length);
}

// ---------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE_TEMPLATE( small_buffer, T, chunker_types, Fixture )
{
    auto chunker = make_test_chunker<T>();

    const std::size_t length = 16;
    std::vector<char> data(length);
    std::generate(data.begin(), data.end(), util::random_generator<char>());

    auto chunks = chunker->chunk(data);
    BOOST_CHECK(!chunks.empty());

    BOOST_CHECK_EQUAL(std::accumulate(chunks.begin(), chunks.end(), 0u), length);
}

// ---------------------------------------------------------------------

} // namespace

