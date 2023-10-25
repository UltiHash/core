//
// Created by masi on 10/24/23.
//

#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "EC tests"
#endif

#include <boost/test/unit_test.hpp>
#include <boost/asio/awaitable.hpp>

#include "global_data_view_fixture.h"



// ------------- Tests Suites Follow --------------

namespace uh::cluster {

BOOST_FIXTURE_TEST_CASE (test_ec, global_data_view_fixture)
{

}

} // end namespace uh::cluster
