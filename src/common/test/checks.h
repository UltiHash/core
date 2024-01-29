#ifndef COMMON_TEST_CHECKS_H
#define COMMON_TEST_CHECKS_H

#include <boost/test/unit_test.hpp>
#include <chrono>


/**
 * Repeatedly check a condition until it evaluates to true or a
 * timeout is hit. Condition is passed to `BOOST_CHECK` for final
 * result.
 *
 * TIMEOUT_MS   timeout in milliseconds
 * CONDITION    condition to evaluate
 */
#define WAIT_UNTIL_CHECK(TIMEOUT_MS, CONDITION) \
    { \
        auto start = std::chrono::steady_clock::now(); \
        do { \
            if ((CONDITION)) { \
                break; \
            } \
        } \
        while ((std::chrono::steady_clock::now() - start) \
                < std::chrono::milliseconds(TIMEOUT_MS)); \
        BOOST_CHECK(CONDITION); \
    } while (false)


#endif
