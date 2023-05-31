//
// Created by masi on 4/26/23.
//
#define BOOST_TEST_MODULE header-only multiunit single test runner
#define BOOST_TEST_NO_MAIN
#define BOOST_TEST_ALTERNATIVE_INIT_API
#include <boost/test/included/unit_test.hpp>

// entry point:
int main(int argc, char* argv[], char* envp[])
{
    return boost::unit_test::unit_test_main( &init_unit_test, argc, argv );
}