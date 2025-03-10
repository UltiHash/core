#define BOOST_TEST_MODULE "reference counter tests"

#include <boost/test/unit_test.hpp>
#include <common/utils/common.h>
#include <lib/util/temp_directory.h>
#include <storage/reference_counter.h>

// ------------- Tests Suites Follow --------------

namespace uh::cluster {

std::size_t dummy_delete(std::size_t offset, std::size_t size) { return 0; }

address make_address(std::size_t offset, std::size_t size) {
    address addr;
    addr.push({offset, size});
    return addr;
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

    address test_addr = make_address(0, DEFAULT_PAGE_SIZE);

    BOOST_CHECK_THROW(refcounter.decrement(test_addr), std::exception);
    BOOST_CHECK(refcounter.increment(test_addr) == test_addr);
    std::deque<reference_counter::refcount_cmd> cmd_queue;
    cmd_queue.emplace_back(reference_counter::INCREMENT, 0, DEFAULT_PAGE_SIZE);
    BOOST_CHECK_NO_THROW(refcounter.execute(cmd_queue));
    BOOST_CHECK(!delete_triggered);
    BOOST_CHECK_NO_THROW(refcounter.decrement(test_addr));
    BOOST_CHECK(delete_triggered);
    BOOST_CHECK_THROW(refcounter.decrement(test_addr), std::exception);
    BOOST_CHECK(refcounter.increment(test_addr) == test_addr);
    cmd_queue.emplace_back(reference_counter::INCREMENT, 0, DEFAULT_PAGE_SIZE);
    BOOST_CHECK_NO_THROW(refcounter.execute(cmd_queue));
}

BOOST_AUTO_TEST_CASE(test_increment_restart_decrement) {
    temp_directory testdir;
    address test_addr = make_address(0, DEFAULT_PAGE_SIZE);
    {
        reference_counter refcounter(testdir.path(), DEFAULT_PAGE_SIZE,
                                     dummy_delete);
        BOOST_CHECK_THROW(refcounter.decrement(test_addr), std::exception);
        BOOST_CHECK(refcounter.increment(test_addr) == test_addr);
        std::deque<reference_counter::refcount_cmd> cmd_queue;
        cmd_queue.emplace_back(reference_counter::INCREMENT, 0,
                               DEFAULT_PAGE_SIZE);
        BOOST_CHECK_NO_THROW(refcounter.execute(cmd_queue));
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
        BOOST_CHECK_NO_THROW(refcounter.decrement(test_addr));
        BOOST_CHECK(delete_triggered);
        BOOST_CHECK_THROW(refcounter.decrement(test_addr), std::exception);
        BOOST_CHECK(refcounter.increment(test_addr) == test_addr);
        std::deque<reference_counter::refcount_cmd> cmd_queue;
        cmd_queue.emplace_back(reference_counter::INCREMENT, 0,
                               DEFAULT_PAGE_SIZE);
        BOOST_CHECK_NO_THROW(refcounter.execute(cmd_queue));
    }
}

BOOST_AUTO_TEST_CASE(test_bulk_increment_decrement) {
    temp_directory testdir;
    address test_addr = make_address(0, GIBI_BYTE);
    std::size_t delete_triggered = 0;
    reference_counter refcounter(
        testdir.path(), DEFAULT_PAGE_SIZE,
        [&delete_triggered](std::size_t offset, std::size_t size) {
            delete_triggered++;
            return 0;
        });
    BOOST_CHECK(refcounter.increment(test_addr) == test_addr);
    std::deque<reference_counter::refcount_cmd> cmd_queue;
    cmd_queue.emplace_back(reference_counter::INCREMENT, 0, GIBI_BYTE);
    BOOST_CHECK_NO_THROW(refcounter.execute(cmd_queue));
    BOOST_CHECK(delete_triggered == 0);
    BOOST_CHECK_NO_THROW(refcounter.decrement(test_addr));
    BOOST_CHECK(delete_triggered == 1);
    BOOST_CHECK_THROW(refcounter.decrement(test_addr), std::exception);
    BOOST_CHECK(refcounter.increment(test_addr) == test_addr);
    cmd_queue.emplace_back(reference_counter::INCREMENT, 0, GIBI_BYTE);
    BOOST_CHECK_NO_THROW(refcounter.execute(cmd_queue));
}

} // end namespace uh::cluster
