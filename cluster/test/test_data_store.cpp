//
// Created by masi on 7/24/23.
//
#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "data_store tests"
#endif

#include <boost/test/unit_test.hpp>
#include "cluster_config.h"
#include "common.h"


// ------------- Tests Suites Follow --------------


namespace uh::cluster {

// ---------------------------------------------------------------------

struct fs_fixture
{

};

// ---------------------------------------------------------------------

BOOST_AUTO_TEST_CASE (test_big_int)
{
    const auto max_ul = std::numeric_limits <unsigned long>::max();
    uint128_t b1 = max_ul / 2 + max_ul / 4 + max_ul / 8;
    uint64_t b2 = max_ul / 2 + max_ul / 4;
    uint8_t answer [] = {0xa7, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfc,
                         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06};
    big_int res = b1 * b2;
    BOOST_TEST(std::memcmp (res.get_data(), answer, sizeof(answer)) == 0);

}
// ---------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE (test_free_spot_manager, fs_fixture)
{

}

// ---------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE (test_data_store, fs_fixture)
{

}
// ---------------------------------------------------------------------

} // end namespace uh::cluster