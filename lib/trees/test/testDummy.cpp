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
#include <trees/tree_radix_custom.h>

// ------------- Tests Follow --------------
BOOST_AUTO_TEST_CASE( constructor )
{
    uh::trees::tree_radix_custom t;
    BOOST_CHECK(!t.has_children());
    uh::trees::tree_radix_custom t_hello("Hello World");
    BOOST_CHECK(!t_hello.has_children());
    BOOST_CHECK(t_hello.size()==11);
    BOOST_CHECK(std::strncmp(t_hello.data_blob(),"Hello World",11)==0);

    char input[11];
    std::strncpy(input,"Hello World",11);
    uh::trees::tree_radix_custom t_hello2(input,11);
    BOOST_CHECK(!t_hello2.has_children());
    BOOST_CHECK(t_hello2.size()==11);
    BOOST_CHECK(std::strncmp(t_hello2.data_blob(),"Hello World",11)==0);
}