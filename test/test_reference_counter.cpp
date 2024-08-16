#define BOOST_TEST_MODULE "reference counter tests"

#include <boost/test/unit_test.hpp>
#include <common/utils/common.h>
#include <common/utils/temp_directory.h>
#include <storage/reference_counter.h>

// ------------- Tests Suites Follow --------------

namespace uh::cluster {

void dummy_delete(std::size_t offset, std::size_t size) {}

BOOST_AUTO_TEST_CASE(test_increment_decrement) {
    temp_directory testdir;
    bool delete_triggered = false;
    reference_counter refcounter(
        testdir.path(), DEFAULT_PAGE_SIZE,
        [&delete_triggered](std::size_t offset, std::size_t size) {
            delete_triggered = true;
        });
    bool success = true;
    BOOST_CHECK_NO_THROW(success = refcounter.increment(0, DEFAULT_PAGE_SIZE));
    BOOST_CHECK(!success);
    BOOST_CHECK_THROW(refcounter.decrement(0, DEFAULT_PAGE_SIZE),
                      std::runtime_error);
    BOOST_CHECK_NO_THROW(success =
                             refcounter.increment(0, DEFAULT_PAGE_SIZE, true));
    BOOST_CHECK(success);
    BOOST_CHECK(!delete_triggered);
    BOOST_CHECK_NO_THROW(refcounter.decrement(0, DEFAULT_PAGE_SIZE));
    BOOST_CHECK(delete_triggered);
    BOOST_CHECK_THROW(refcounter.decrement(0, DEFAULT_PAGE_SIZE),
                      std::runtime_error);
    BOOST_CHECK_NO_THROW(success = refcounter.increment(0, DEFAULT_PAGE_SIZE));
    BOOST_CHECK(!success);
}

BOOST_AUTO_TEST_CASE(test_increment_restart_decrement) {
    temp_directory testdir;
    {
        reference_counter refcounter(testdir.path(), DEFAULT_PAGE_SIZE,
                                     dummy_delete);
        bool success = true;
        BOOST_CHECK_NO_THROW(success =
                                 refcounter.increment(0, DEFAULT_PAGE_SIZE));
        BOOST_CHECK(!success);
        BOOST_CHECK_THROW(refcounter.decrement(0, DEFAULT_PAGE_SIZE),
                          std::runtime_error);
        BOOST_CHECK_NO_THROW(
            success = refcounter.increment(0, DEFAULT_PAGE_SIZE, true));
        BOOST_CHECK(success);
    }
    {
        bool delete_triggered = false;
        reference_counter refcounter(
            testdir.path(), DEFAULT_PAGE_SIZE,
            [&delete_triggered](std::size_t offset, std::size_t size) {
                delete_triggered = true;
            });
        BOOST_CHECK(!delete_triggered);
        BOOST_CHECK_NO_THROW(refcounter.decrement(0, DEFAULT_PAGE_SIZE));
        BOOST_CHECK(delete_triggered);
        BOOST_CHECK_THROW(refcounter.decrement(0, DEFAULT_PAGE_SIZE),
                          std::runtime_error);
        bool success = true;
        BOOST_CHECK_NO_THROW(success =
                                 refcounter.increment(0, DEFAULT_PAGE_SIZE));
        BOOST_CHECK(!success);
    }
}

BOOST_AUTO_TEST_CASE(test_bulk_increment_decrement) {
    temp_directory testdir;
    std::size_t delete_triggered = 0;
    reference_counter refcounter(
        testdir.path(), DEFAULT_PAGE_SIZE,
        [&delete_triggered](std::size_t offset, std::size_t size) {
            delete_triggered++;
        });
    bool success = true;
    BOOST_CHECK_NO_THROW(success = refcounter.increment(0, GIBI_BYTE));
    BOOST_CHECK(!success);
    BOOST_CHECK_NO_THROW(success = refcounter.increment(0, GIBI_BYTE, true));
    BOOST_CHECK(success);
    BOOST_CHECK(delete_triggered == 0);
    BOOST_CHECK_NO_THROW(refcounter.decrement(0, GIBI_BYTE));
    BOOST_CHECK(delete_triggered == 1);
    BOOST_CHECK_THROW(refcounter.decrement(0, GIBI_BYTE), std::runtime_error);
    BOOST_CHECK_NO_THROW(success = refcounter.increment(0, GIBI_BYTE));
    BOOST_CHECK(!success);
}

} // end namespace uh::cluster
