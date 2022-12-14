//
// Created by max on 02.11.22.
//

#define BOOST_TEST_DYN_LINK
#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "uhLibDBSupport Dummy Tests"
#endif

#include <boost/test/unit_test.hpp>
#include <util/recomp_tree.h>


// ------------- Tests Follow --------------
BOOST_AUTO_TEST_CASE( dummy )
{
    BOOST_CHECK(true);
}