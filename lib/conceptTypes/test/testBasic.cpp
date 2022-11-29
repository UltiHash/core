//
// Created by max on 02.11.22.
//

#define BOOST_TEST_DYN_LINK
#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "uhLibConceptTypes type_name, const index and reference detection Tests"
#endif

#include <boost/test/unit_test.hpp>
#include <conceptTypes/conceptTypes.h>

// ------------- Tests Follow --------------
BOOST_AUTO_TEST_CASE( typename_test )
{
    BOOST_TEST_MESSAGE("Checking type_name function: ");
    BOOST_CHECK_EQUAL("int&",type_name<int&>());
    BOOST_CHECK_EQUAL("int&&",type_name<int&&>());
    BOOST_CHECK_EQUAL("std::vector<int>",type_name<std::vector<int>>());
    BOOST_CHECK_EQUAL("std::vector<int>&",type_name<std::vector<int>&>());
    BOOST_CHECK_EQUAL("std::vector<int>&&",type_name<std::vector<int>&&>());
    std::cout << std::endl;
}

BOOST_AUTO_TEST_CASE( indexer_test )
{
    BOOST_TEST_MESSAGE("Checking indexer struct: ");
    using test_tup=boost::tuple<std::vector<int>,int>;
    BOOST_CHECK_EQUAL("boost::tuples::tuple<std::vector<int>, int>",type_name<test_tup>());
    //Checking position of std::vector<int> in test tuple:
    int pos=indexer<std::vector<int>,test_tup>::value;
    BOOST_CHECK_EQUAL(0,pos);
    int pos2=indexer<int,test_tup>::value;
    BOOST_CHECK_EQUAL(1,pos2);
    std::cout << std::endl;
}

BOOST_AUTO_TEST_CASE( reference_type_test )
{
    BOOST_TEST_MESSAGE("Typename tester for concepts ClearValue, LValue and RValue: ");
    BOOST_TEST_MESSAGE("ClearValue: ");
    BOOST_CHECK_EQUAL(ClearValue<int>,1);
    BOOST_CHECK_EQUAL(ClearValue<int&>,0);
    BOOST_CHECK_EQUAL(ClearValue<int&&>,0);
    BOOST_TEST_MESSAGE("ClearValue: ");
    BOOST_CHECK_EQUAL(LValue<int>,0);
    BOOST_CHECK_EQUAL(LValue<int&>,1);
    BOOST_CHECK_EQUAL(LValue<int&&>,0);
    BOOST_TEST_MESSAGE("ClearValue: ");
    BOOST_CHECK_EQUAL(RValue<int>,0);
    BOOST_CHECK_EQUAL(RValue<int&>,0);
    BOOST_CHECK_EQUAL(RValue<int&&>,1);
    std::cout << std::endl;
}