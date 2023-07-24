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

// ------------- Tests Suites Follow --------------


namespace uh::cluster {

// ---------------------------------------------------------------------

struct fs_fixture
{

};

// ---------------------------------------------------------------------

BOOST_AUTO_TEST_CASE (test_big_int)
{

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