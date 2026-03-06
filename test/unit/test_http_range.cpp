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

#define BOOST_TEST_MODULE "tests for http ranges"

#include <entrypoint/http/range.h>

#include <boost/test/unit_test.hpp>

namespace uh::cluster::ep::http {

BOOST_AUTO_TEST_CASE(parse_range_header_start_end) {

    auto spec = parse_range_header("bytes=100-899", 1000);

    BOOST_CHECK(spec.unit == range_spec::bytes);
    BOOST_CHECK_EQUAL(spec.ranges.size(), 1ull);
    BOOST_CHECK_EQUAL(spec.ranges.front().start, 100);
    BOOST_CHECK_EQUAL(spec.ranges.front().end, 900);
    BOOST_CHECK_EQUAL(spec.ranges.front().length(), 800);
}

BOOST_AUTO_TEST_CASE(parse_range_header_open_end) {

    auto spec = parse_range_header("bytes=100-", 1000);

    BOOST_CHECK(spec.unit == range_spec::bytes);
    BOOST_CHECK_EQUAL(spec.ranges.size(), 1ull);
    BOOST_CHECK_EQUAL(spec.ranges.front().start, 100);
    BOOST_CHECK_EQUAL(spec.ranges.front().end, 1000);
    BOOST_CHECK_EQUAL(spec.ranges.front().length(), 900);
}

BOOST_AUTO_TEST_CASE(parse_range_header_negative_offset) {

    auto spec = parse_range_header("bytes=-100", 1000);

    BOOST_CHECK(spec.unit == range_spec::bytes);
    BOOST_CHECK_EQUAL(spec.ranges.size(), 1ull);
    BOOST_CHECK_EQUAL(spec.ranges.front().start, 900);
    BOOST_CHECK_EQUAL(spec.ranges.front().end, 1000);
    BOOST_CHECK_EQUAL(spec.ranges.front().length(), 100);
}

} // namespace uh::cluster::ep::http
