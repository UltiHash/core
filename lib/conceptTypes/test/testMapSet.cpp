//
// Created by benjamin-elias on 26.11.22.
//

#define BOOST_TEST_DYN_LINK
#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "uhLibConceptTypes map and set container filtering Tests"
#endif

#include <boost/test/unit_test.hpp>
#include <conceptTypes/conceptTypes.h>

// ------------- Tests Follow --------------

BOOST_AUTO_TEST_CASE( map_test )
{
    BOOST_TEST_MESSAGE("Typename tester for concept Map: ");
    bool b=Map<std::map<int,std::string>>;
    BOOST_CHECK_EQUAL(b,1);
    b=Map<boost::container::map<int,std::string>>;
    BOOST_CHECK_EQUAL(b,1);
    b=Map<boost::fusion::map<int,std::string>>;
    BOOST_CHECK_EQUAL(b,1);
    b=Map<std::vector<int>>;
    BOOST_CHECK_EQUAL(b,0);
    b=Map<int>;
    BOOST_CHECK_EQUAL(b,0);
    BOOST_TEST_MESSAGE("Typename tester for concept Map with references for std::map: ");
    b=Map<std::map<int,std::string>>;
    BOOST_CHECK_EQUAL(b,1);
    b=Map<std::map<int,std::string>&>;
    BOOST_CHECK_EQUAL(b,1);
    b=Map<std::map<int,std::string>&&>;
    BOOST_CHECK_EQUAL(b,1);
    b=Map<const std::map<int,std::string>>;
    BOOST_CHECK_EQUAL(b,1);
    b=Map<const std::map<int,std::string>&>;
    BOOST_CHECK_EQUAL(b,1);
    b=Map<const std::map<int,std::string>&&>;
    BOOST_CHECK_EQUAL(b,1);
    BOOST_TEST_MESSAGE("Typename tester for concept Map with references for boost::unordered_map: ");
    b=Map<boost::unordered_map<int,std::string>>;
    BOOST_CHECK_EQUAL(b,1);
    b=Map<boost::unordered_map<int,std::string>&>;
    BOOST_CHECK_EQUAL(b,1);
    b=Map<boost::unordered_map<int,std::string>&&>;
    BOOST_CHECK_EQUAL(b,1);
    b=Map<const boost::unordered_map<int,std::string>>;
    BOOST_CHECK_EQUAL(b,1);
    b=Map<const boost::unordered_map<int,std::string>&>;
    BOOST_CHECK_EQUAL(b,1);
    b=Map<const boost::unordered_map<int,std::string>&&>;
    BOOST_CHECK_EQUAL(b,1);
    BOOST_TEST_MESSAGE("Typename tester for concept Map with references for boost::container::map: ");
    b=Map<boost::container::map<int,std::string>>;
    BOOST_CHECK_EQUAL(b,1);
    b=Map<boost::container::map<int,std::string>&>;
    BOOST_CHECK_EQUAL(b,1);
    b=Map<boost::container::map<int,std::string>&&>;
    BOOST_CHECK_EQUAL(b,1);
    b=Map<const boost::container::map<int,std::string>>;
    BOOST_CHECK_EQUAL(b,1);
    b=Map<const boost::container::map<int,std::string>&>;
    BOOST_CHECK_EQUAL(b,1);
    b=Map<const boost::container::map<int,std::string>&&>;
    BOOST_CHECK_EQUAL(b,1);
    BOOST_TEST_MESSAGE("Typename tester for concept Map with references for boost::fusion::map: ");
    b=Map<boost::fusion::map<int,std::string>>;
    BOOST_CHECK_EQUAL(b,1);
    b=Map<boost::fusion::map<int,std::string>&>;
    BOOST_CHECK_EQUAL(b,1);
    b=Map<boost::fusion::map<int,std::string>&&>;
    BOOST_CHECK_EQUAL(b,1);
    b=Map<const boost::fusion::map<int,std::string>>;
    BOOST_CHECK_EQUAL(b,1);
    b=Map<const boost::fusion::map<int,std::string>&>;
    BOOST_CHECK_EQUAL(b,1);
    b=Map<const boost::fusion::map<int,std::string>&&>;
    BOOST_CHECK_EQUAL(b,1);
}

BOOST_AUTO_TEST_CASE( set_test )
{
    BOOST_TEST_MESSAGE("Typename tester for concept Set: ");
    bool b=Set<std::set<int,std::string>>;
    BOOST_CHECK_EQUAL(b,1);
    b=Set<boost::container::set<int,std::string>>;
    BOOST_CHECK_EQUAL(b,1);
    b=Set<boost::fusion::set<int,std::string>>;
    BOOST_CHECK_EQUAL(b,1);
    b=Set<std::vector<int>>;
    BOOST_CHECK_EQUAL(b,0);
    b=Set<int>;
    BOOST_CHECK_EQUAL(b,0);
    BOOST_TEST_MESSAGE("Typename tester for concept Set with references for std::set: ");
    b=Set<std::set<int,std::string>>;
    BOOST_CHECK_EQUAL(b,1);
    b=Set<std::set<int,std::string>&>;
    BOOST_CHECK_EQUAL(b,1);
    b=Set<std::set<int,std::string>&&>;
    BOOST_CHECK_EQUAL(b,1);
    b=Set<const std::set<int,std::string>>;
    BOOST_CHECK_EQUAL(b,1);
    b=Set<const std::set<int,std::string>&>;
    BOOST_CHECK_EQUAL(b,1);
    b=Set<const std::set<int,std::string>&&>;
    BOOST_CHECK_EQUAL(b,1);
    BOOST_TEST_MESSAGE("Typename tester for concept Set with references for boost::container::set: ");
    b=Set<boost::container::set<int,std::string>>;
    BOOST_CHECK_EQUAL(b,1);
    b=Set<boost::container::set<int,std::string>&>;
    BOOST_CHECK_EQUAL(b,1);
    b=Set<boost::container::set<int,std::string>&&>;
    BOOST_CHECK_EQUAL(b,1);
    b=Set<const boost::container::set<int,std::string>>;
    BOOST_CHECK_EQUAL(b,1);
    b=Set<const boost::container::set<int,std::string>&>;
    BOOST_CHECK_EQUAL(b,1);
    b=Set<const boost::container::set<int,std::string>&&>;
    BOOST_CHECK_EQUAL(b,1);
    BOOST_TEST_MESSAGE("Typename tester for concept Set with references for boost::fusion::set: ");
    b=Set<boost::fusion::set<int,std::string>>;
    BOOST_CHECK_EQUAL(b,1);
    b=Set<boost::fusion::set<int,std::string>&>;
    BOOST_CHECK_EQUAL(b,1);
    b=Set<boost::fusion::set<int,std::string>&&>;
    BOOST_CHECK_EQUAL(b,1);
    b=Set<const boost::fusion::set<int,std::string>>;
    BOOST_CHECK_EQUAL(b,1);
    b=Set<const boost::fusion::set<int,std::string>&>;
    BOOST_CHECK_EQUAL(b,1);
    b=Set<const boost::fusion::set<int,std::string>&&>;
    BOOST_CHECK_EQUAL(b,1);
}