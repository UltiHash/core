#define BOOST_TEST_MODULE "reference counter tests"

#include <boost/test/unit_test.hpp>

#include <common/utils/common.h>
#include <storage/reference_counter.h>

#include <util/temp_directory.h>

// ------------- Tests Suites Follow --------------

namespace uh::cluster {

std::size_t dummy_delete(std::size_t offset, std::size_t size) { return 0; }

std::vector<refcount_t> make_refcounts(std::size_t offset, std::size_t size) {
    std::vector<refcount_t> refcounts;

    std::size_t first_stripe = offset / DEFAULT_PAGE_SIZE;
    std::size_t last_stripe = (offset + size - 1) / DEFAULT_PAGE_SIZE;
    for (size_t stripe_id = first_stripe; stripe_id <= last_stripe;
         stripe_id++) {
        refcounts.emplace_back(stripe_id, 1);
    }
    return refcounts;
}

BOOST_AUTO_TEST_CASE(test_increment_decrement) {
    temp_directory testdir;
    bool delete_triggered = false;
    reference_counter refcounter(
        testdir.path(), DEFAULT_PAGE_SIZE,
        [&delete_triggered](std::size_t offset, std::size_t size) {
            delete_triggered = true;
            return 0;
        });

    std::vector<refcount_t> test_refcounts =
        make_refcounts(0, DEFAULT_PAGE_SIZE);

    BOOST_CHECK_THROW(refcounter.decrement(test_refcounts), std::exception);
    BOOST_CHECK((refcounter.increment(test_refcounts) == test_refcounts));
    BOOST_CHECK(refcounter.increment(test_refcounts, 0).empty());
    BOOST_CHECK(!delete_triggered);
    BOOST_CHECK_NO_THROW(refcounter.decrement(test_refcounts));
    BOOST_CHECK(delete_triggered);
    BOOST_CHECK_THROW(refcounter.decrement(test_refcounts), std::exception);
    BOOST_CHECK((refcounter.increment(test_refcounts) == test_refcounts));
    BOOST_CHECK_NO_THROW(refcounter.increment(test_refcounts, 0));
}

BOOST_AUTO_TEST_CASE(test_increment_restart_decrement) {
    temp_directory testdir;
    auto test_refcounts = make_refcounts(0, DEFAULT_PAGE_SIZE);
    {
        reference_counter refcounter(testdir.path(), DEFAULT_PAGE_SIZE,
                                     dummy_delete);
        BOOST_CHECK_THROW(refcounter.decrement(test_refcounts), std::exception);
        BOOST_CHECK(refcounter.increment(test_refcounts) == test_refcounts);
        BOOST_CHECK_NO_THROW(refcounter.increment(test_refcounts, 0));
    }
    {
        bool delete_triggered = false;
        reference_counter refcounter(
            testdir.path(), DEFAULT_PAGE_SIZE,
            [&delete_triggered](std::size_t offset, std::size_t size) {
                delete_triggered = true;
                return 0;
            });
        BOOST_CHECK(!delete_triggered);
        BOOST_CHECK_NO_THROW(refcounter.decrement(test_refcounts));
        BOOST_CHECK(delete_triggered);
        BOOST_CHECK_THROW(refcounter.decrement(test_refcounts), std::exception);
        BOOST_CHECK(refcounter.increment(test_refcounts) == test_refcounts);
        BOOST_CHECK_NO_THROW(refcounter.increment(test_refcounts, 0));
    }
}

BOOST_AUTO_TEST_CASE(test_bulk_increment_decrement) {
    temp_directory testdir;
    auto test_refcounts = make_refcounts(0, MEBI_BYTE);
    std::size_t delete_triggered = 0;
    reference_counter refcounter(
        testdir.path(), DEFAULT_PAGE_SIZE,
        [&delete_triggered](std::size_t offset, std::size_t size) {
            delete_triggered++;
            return 0;
        });
    BOOST_CHECK(refcounter.increment(test_refcounts) == test_refcounts);
    BOOST_CHECK_NO_THROW(refcounter.increment(test_refcounts, 0));
    BOOST_CHECK(delete_triggered == 0);
    BOOST_CHECK_NO_THROW(refcounter.decrement(test_refcounts));
    BOOST_CHECK(delete_triggered == 1);
    BOOST_CHECK_THROW(refcounter.decrement(test_refcounts), std::exception);
    BOOST_CHECK(refcounter.increment(test_refcounts) == test_refcounts);
    BOOST_CHECK_NO_THROW(refcounter.increment(test_refcounts, 0));
}

} // end namespace uh::cluster
