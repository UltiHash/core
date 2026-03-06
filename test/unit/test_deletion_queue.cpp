// Copyright 2026 UltiHash Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#define BOOST_TEST_MODULE "deletion_queue tests"

#include <util/proxy_cache_config.h>

#include <proxy/cache/disk/deletion_queue.h>

#include <memory>

#include "test_config.h"

namespace uh::cluster::proxy::cache {

BOOST_AUTO_TEST_SUITE(a_deletion_queue)

BOOST_AUTO_TEST_CASE(supports_push_and_pop_items) {
    auto queue = disk::deletion_queue<s3_object_key, char_vector>();
    queue.push(char_vector::create({1, 2, 3}));
    queue.push(char_vector::create({4, 5, 6, 7}));
    queue.push(char_vector::create({8, 9, 10, 11, 12}));
    auto items = queue.pop(5);
    BOOST_CHECK_EQUAL(queue.data_size(), 5);
    BOOST_CHECK_EQUAL(items.size(), 2);
    BOOST_CHECK_EQUAL((*items[0]).data_size(), 3);
    BOOST_CHECK_EQUAL((*items[1]).data_size(), 4);
    BOOST_CHECK_EQUAL((*items[0])[0], 1);
    BOOST_CHECK_EQUAL((*items[1])[0], 4);
}

BOOST_AUTO_TEST_CASE(supports_concurrent_push) {
    auto queue = disk::deletion_queue<s3_object_key, char_vector>();

    const int thread_count = 10;
    const char items_per_thread = 100;

    std::vector<std::thread> threads;

    for (int t = 0; t < thread_count; ++t) {
        threads.emplace_back([t, &queue]() {
            for (char i = 0; i < items_per_thread; ++i) {
                s3_object_key key{"bucket" + std::to_string(t),
                                  "object" + std::to_string(i), "v1"};
                auto e = char_vector::create({i});
                queue.push(std::move(e));
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    BOOST_CHECK_EQUAL(queue.data_size(), 1000); // sum of 0..99, ten times

    auto items = queue.pop(1200);
    BOOST_CHECK_EQUAL(items.size(), 1000);
    BOOST_CHECK_EQUAL(queue.data_size(), 0);
}

BOOST_AUTO_TEST_CASE(supports_concurrent_push_and_pop) {
    auto queue = disk::deletion_queue<s3_object_key, char_vector>();

    const int thread_count = 10;
    const int items_per_thread = 100;

    // Push threads
    std::vector<std::thread> push_threads;
    for (int t = 0; t < thread_count; ++t) {
        push_threads.emplace_back([t, &queue]() {
            for (char i = 0; i < items_per_thread; ++i) {
                s3_object_key key{"bucket" + std::to_string(t),
                                  "object" + std::to_string(i), "v1"};
                auto e = char_vector::create({i});
                queue.push(std::move(e));
            }
        });
    }

    // Pop threads
    std::vector<std::vector<std::shared_ptr<char_vector>>> pop_results(
        thread_count);
    std::vector<std::thread> pop_threads;
    for (int t = 0; t < thread_count; ++t) {
        pop_threads.emplace_back(
            [t, &queue, &pop_results]() { pop_results[t] = queue.pop(10); });
    }

    for (auto& thread : push_threads)
        thread.join();
    for (auto& thread : pop_threads)
        thread.join();

    size_t total_popped = 0;
    for (const auto& vec : pop_results)
        total_popped += vec.size();

    auto remaining = queue.pop(1200);
    total_popped += remaining.size();

    BOOST_CHECK_EQUAL(total_popped, thread_count * items_per_thread);
    BOOST_CHECK_EQUAL(queue.data_size(), 0);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace uh::cluster::proxy::cache
