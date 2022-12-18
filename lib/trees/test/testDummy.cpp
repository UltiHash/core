//
// Created by benjamin on 18.12.22.
//

#define BOOST_TEST_DYN_LINK
#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "uhLibTrees Dummy Tests"
#endif

#include <boost/test/unit_test.hpp>
#include <logging/logging_boost.h>

// ------------- Tests Follow --------------
BOOST_AUTO_TEST_CASE( dummy )
{
    BOOST_CHECK(true);
}