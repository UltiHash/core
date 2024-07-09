#define BOOST_TEST_MODULE "reference counter tests"

#include <boost/test/unit_test.hpp>
#include <storage/reference_counter.h>
#include <common/utils/temp_directory.h>
#include <common/utils/common.h>
#include <deduplicator/config.h>

// ------------- Tests Suites Follow --------------

namespace uh::cluster {


BOOST_AUTO_TEST_CASE(test_increment_decrement) {
    temp_directory testdir;
    reference_counter refcounter(testdir.path());
    BOOST_CHECK_THROW(refcounter.decrement({0}), std::runtime_error);
    BOOST_CHECK_THROW(refcounter.increment({0}), std::runtime_error);
    refcounter.initialize({0});
    BOOST_CHECK(refcounter.increment({0}).at(0) == 2);
    BOOST_CHECK(refcounter.decrement({0}).at(0) == 1);
    BOOST_CHECK(refcounter.decrement({0}).at(0) == 0);
    BOOST_CHECK_THROW(refcounter.decrement({0}), std::runtime_error);
    BOOST_CHECK(refcounter.increment({0}).at(0) == 1);
}

BOOST_AUTO_TEST_CASE(test_increment_restart_decrement) {
    temp_directory testdir;
    {
        reference_counter refcounter(testdir.path());
        BOOST_CHECK_THROW(refcounter.decrement({0}), std::runtime_error);
        BOOST_CHECK_THROW(refcounter.increment({0}), std::runtime_error);
        refcounter.initialize({0});
        BOOST_CHECK(refcounter.increment({0}).at(0) == 2);
    }
    {
        reference_counter refcounter(testdir.path());
        BOOST_CHECK(refcounter.decrement({0}).at(0) == 1);
        BOOST_CHECK(refcounter.decrement({0}).at(0) == 0);
        BOOST_CHECK_THROW(refcounter.decrement({0}).at(0), std::runtime_error);
        BOOST_CHECK(refcounter.increment({0}).at(0) == 1);
    }
}

BOOST_AUTO_TEST_CASE(test_bulk_increment_decrement) {
    temp_directory testdir;
    reference_counter refcounter(testdir.path());
    deduplicator_config cfg;
    const std::size_t num_pages = GIBI_BYTE / cfg.max_fragment_size;
    std::set<std::size_t> pages;
    for(std::size_t i = 0; i < num_pages; i++) {
        pages.insert(i);
    }
    BOOST_CHECK_THROW(refcounter.increment(pages), std::runtime_error);
    refcounter.initialize(pages);
    refcounter.increment(pages);
    refcounter.decrement(pages);
    refcounter.decrement(pages);
    BOOST_CHECK_THROW(refcounter.decrement(pages), std::runtime_error);
}

} // end namespace uh::cluster
