/*
 * Copyright 2026 UltiHash Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

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
#define WAIT_UNTIL_CHECK(TIMEOUT_MS, CONDITION)                                \
    do {                                                                       \
        auto start = std::chrono::steady_clock::now();                         \
        do {                                                                   \
            if ((CONDITION)) {                                                 \
                break;                                                         \
            }                                                                  \
        } while ((std::chrono::steady_clock::now() - start) <                  \
                 std::chrono::milliseconds(TIMEOUT_MS));                       \
        BOOST_CHECK(CONDITION);                                                \
    } while (false)

/**
 * Repeatedly execute a statement until it does not throw an
 * exception or when a timeout is hit. The call evaluates to
 * true when the statement could be executed without exception
 * once.
 *
 * TIMEOUT_MS   timeout in milliseconds
 * STATEMENT    statement to execute
 */
#define WAIT_UNTIL_NO_THROW(TIMEOUT_MS, STATEMENT)                             \
    do {                                                                       \
        auto start = std::chrono::steady_clock::now();                         \
        bool success = false;                                                  \
        do {                                                                   \
            try {                                                              \
                STATEMENT;                                                     \
                success = true;                                                \
                break;                                                         \
            } catch (...) {                                                    \
            }                                                                  \
        } while ((std::chrono::steady_clock::now() - start) <                  \
                 std::chrono::milliseconds(TIMEOUT_MS));                       \
        BOOST_CHECK(success);                                                  \
    } while (false)

/**
 * Repeatedly check a condition until a timeout is hit. `BOOST_CHECK`
 * passes if a 'true' condition was passed and it was not changed during
 * the entire elapsed time.
 *
 * TIMEOUT_MS   timeout in milliseconds
 * CONDITION    condition to evaluate. must be 'true' initially
 */
#define CHECK_STABLE(TIMEOUT_MS, CONDITION)                                    \
    do {                                                                       \
        auto start = std::chrono::steady_clock::now();                         \
        bool success = true;                                                   \
        do {                                                                   \
            if (!(CONDITION)) {                                                \
                success = false;                                               \
                break;                                                         \
            }                                                                  \
        } while ((std::chrono::steady_clock::now() - start) <                  \
                 std::chrono::milliseconds(TIMEOUT_MS));                       \
        BOOST_CHECK(success);                                                  \
    } while (false)
