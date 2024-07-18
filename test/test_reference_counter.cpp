#define BOOST_TEST_MODULE "reference counter tests"

#include <boost/test/unit_test.hpp>
#include <storage/reference_counter.h>
#include <common/utils/temp_directory.h>
#include <common/utils/common.h>

// ------------- Tests Suites Follow --------------

namespace uh::cluster {


BOOST_AUTO_TEST_CASE(test_increment_decrement) {
    temp_directory testdir;
    reference_counter refcounter(testdir.path(), DEFAULT_PAGE_SIZE);
    BOOST_CHECK_THROW(refcounter.decrement(0, DEFAULT_PAGE_SIZE), std::runtime_error);
    BOOST_CHECK(refcounter.increment(0, DEFAULT_PAGE_SIZE).at(0) == 1);
    BOOST_CHECK(refcounter.decrement(0, DEFAULT_PAGE_SIZE).at(0) == 0);
    BOOST_CHECK_THROW(refcounter.decrement(0, DEFAULT_PAGE_SIZE), std::runtime_error);
    BOOST_CHECK(refcounter.increment(0, DEFAULT_PAGE_SIZE).at(0) == 1);
}

BOOST_AUTO_TEST_CASE(test_increment_restart_decrement) {
    temp_directory testdir;
    {
        reference_counter refcounter(testdir.path(), DEFAULT_PAGE_SIZE);
        BOOST_CHECK_THROW(refcounter.decrement(0, DEFAULT_PAGE_SIZE), std::runtime_error);
        BOOST_CHECK(refcounter.increment(0, DEFAULT_PAGE_SIZE).at(0) == 1);
    }
    {
        reference_counter refcounter(testdir.path(), DEFAULT_PAGE_SIZE);
        BOOST_CHECK(refcounter.decrement(0, DEFAULT_PAGE_SIZE).at(0) == 0);
        BOOST_CHECK_THROW(refcounter.decrement(0, DEFAULT_PAGE_SIZE).at(0), std::runtime_error);
        BOOST_CHECK(refcounter.increment(0, DEFAULT_PAGE_SIZE).at(0) == 1);
    }
}

BOOST_AUTO_TEST_CASE(test_bulk_increment_decrement) {
    temp_directory testdir;
    reference_counter refcounter(testdir.path(), DEFAULT_PAGE_SIZE);
    refcounter.increment(0, GIBI_BYTE);
    refcounter.decrement(0, GIBI_BYTE);
    BOOST_CHECK_THROW(refcounter.decrement(0, GIBI_BYTE), std::runtime_error);
}

} // end namespace uh::cluster
