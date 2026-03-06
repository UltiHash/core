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

#define BOOST_TEST_MODULE "plain cache tests"

#include "common/caches/plain_cache.h"
#include <boost/test/unit_test.hpp>

using namespace uh::cluster;

constexpr std::string_view data1 =
    "d41d8cd98f00b204e9800998ecf8427ed41d8cd98f00b204e9800998ecab32f";
BOOST_AUTO_TEST_CASE(data_sanity) {
    int failures = 0;
    plain_cache pc(32, [&failures](std::span<char> str) {
        auto [in, out] = zpp::bits::in_out(str);
        while (in.position() < str.size()) {
            std::string data;
            in(data).or_throw();
            if (data != data1) {
                failures++;
            }
        }
    });

    for (int i = 0; i < 64; ++i) {
        pc << data1;
    }

    BOOST_TEST(failures == 0);
}

BOOST_AUTO_TEST_CASE(callback_count) {
    int callbacks = 0;
    plain_cache pc(32, [&callbacks](std::span<char> str) { callbacks++; });

    for (int i = 0; i < 640; ++i) {
        pc << data1;
    }

    BOOST_TEST(callbacks == 20);
}

BOOST_AUTO_TEST_CASE(destruction) {
    int callbacks = 0;
    {
        plain_cache pc(32, [&callbacks](std::span<char> str) { callbacks++; });

        for (int i = 0; i < 33; ++i) {
            pc << data1;
        }
    }

    BOOST_TEST(callbacks == 2);
}