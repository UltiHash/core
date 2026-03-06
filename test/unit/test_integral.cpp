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

#define BOOST_TEST_MODULE "boost alignment tests"

#include <boost/test/unit_test.hpp>

#include "test_config.h"

#include <common/utils/integral.h>

namespace uh::cluster {

BOOST_AUTO_TEST_SUITE(a_integral_utility)

BOOST_AUTO_TEST_CASE(divides_and_ceils) {
    const auto chunk_size = 10;
    BOOST_TEST(div_ceil<std::size_t>(0ul, chunk_size) == 0);
    BOOST_TEST(div_ceil<std::size_t>(1ul, chunk_size) == 1);
    BOOST_TEST(div_ceil<std::size_t>(9ul, chunk_size) == 1);
    BOOST_TEST(div_ceil<std::size_t>(10ul, chunk_size) == 1);
    BOOST_TEST(div_ceil<std::size_t>(11ul, chunk_size) == 2);
}

BOOST_AUTO_TEST_CASE(aligns_up) {
    const auto chunk_size = 10;
    BOOST_TEST(align_up<std::size_t>(0ul, chunk_size) == 0);
    BOOST_TEST(align_up<std::size_t>(1ul, chunk_size) == chunk_size);
    BOOST_TEST(align_up<std::size_t>(9ul, chunk_size) == chunk_size);
    BOOST_TEST(align_up<std::size_t>(10ul, chunk_size) == chunk_size);
    BOOST_TEST(align_up<std::size_t>(11ul, chunk_size) == 2 * chunk_size);
}

BOOST_AUTO_TEST_CASE(divides_and_floors) {
    const auto chunk_size = 10;
    BOOST_TEST(div_floor<std::size_t>(0ul, chunk_size) == 0);
    BOOST_TEST(div_floor<std::size_t>(1ul, chunk_size) == 0);
    BOOST_TEST(div_floor<std::size_t>(9ul, chunk_size) == 0);
    BOOST_TEST(div_floor<std::size_t>(10ul, chunk_size) == 1);
    BOOST_TEST(div_floor<std::size_t>(11ul, chunk_size) == 1);
}

BOOST_AUTO_TEST_CASE(aligns_up_next) {
    const auto chunk_size = 10;
    BOOST_TEST(align_up_next<std::size_t>(0ul, chunk_size) == chunk_size);
    BOOST_TEST(align_up_next<std::size_t>(1ul, chunk_size) == chunk_size);
    BOOST_TEST(align_up_next<std::size_t>(9ul, chunk_size) == chunk_size);
    BOOST_TEST(align_up_next<std::size_t>(10ul, chunk_size) == 2 * chunk_size);
    BOOST_TEST(align_up_next<std::size_t>(11ul, chunk_size) == 2 * chunk_size);
}

// while (p < pointer + read_size) {
//     auto next_p = align_up<uint128_t>(p + 1, m_chunk_size);
//     next_p = std::min(next_p, pointer + read_size);
//     auto frag = fragment{.pointer = p,
//                          .size = static_cast<std::size_t>(next_p - p)};
//     addr.push(frag);
//     p = next_p;
// }

BOOST_AUTO_TEST_SUITE_END()

} // namespace uh::cluster
