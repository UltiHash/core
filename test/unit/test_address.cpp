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

#define BOOST_TEST_MODULE "address tests"

#include "test_config.h"

#include <boost/test/unit_test.hpp>
#include <common/types/address.h>

// ------------- Tests Suites Follow --------------

namespace uh::cluster {

BOOST_AUTO_TEST_CASE(subfrag) {
    fragment f{0, 100};

    {
        auto sub = f.subfrag(0);
        BOOST_CHECK_EQUAL(sub.pointer, f.pointer);
        BOOST_CHECK_EQUAL(sub.size, f.size);
    }

    {
        auto sub = f.subfrag(20);
        BOOST_CHECK_EQUAL(sub.pointer, 20);
        BOOST_CHECK_EQUAL(sub.size, f.size - 20);
    }

    {
        auto sub = f.subfrag(20, 30);
        BOOST_CHECK_EQUAL(sub.pointer, 20);
        BOOST_CHECK_EQUAL(sub.size, 10);
    }

    {
        auto sub = f.subfrag(80, 350);
        BOOST_CHECK_EQUAL(sub.pointer, 80);
        BOOST_CHECK_EQUAL(sub.size, 20);
    }
}

BOOST_AUTO_TEST_CASE(range) {
    address a;
    a.push({0, 100});

    {
        address b = a.range(20, 30);
        BOOST_CHECK_EQUAL(b.data_size(), 10);
        BOOST_CHECK_EQUAL(b.size(), 1);
        BOOST_CHECK_EQUAL(b.get(0).pointer, 20);
        BOOST_CHECK_EQUAL(b.get(0).size, 10);
    }

    {
        address b = a.range(200, 100);
        BOOST_CHECK_EQUAL(b.data_size(), 0);
        BOOST_CHECK_EQUAL(b.size(), 0);
    }

    {
        address b = a.range(20, 100);
        BOOST_CHECK_EQUAL(b.data_size(), 80);
        BOOST_CHECK_EQUAL(b.size(), 1);
        BOOST_CHECK_EQUAL(b.get(0).pointer, 20);
        BOOST_CHECK_EQUAL(b.get(0).size, 80);
    }

    a.push({30, 200});
    a.push({30, 200});

    {
        address c = a.range(80, 350);
        BOOST_CHECK_EQUAL(c.data_size(), 270);
        BOOST_CHECK_EQUAL(c.size(), 3);
        BOOST_CHECK_EQUAL(c.get(0).pointer, 80);
        BOOST_CHECK_EQUAL(c.get(0).size, 20);
        BOOST_CHECK_EQUAL(c.get(1).pointer, 30);
        BOOST_CHECK_EQUAL(c.get(1).size, 200);
        BOOST_CHECK_EQUAL(c.get(2).pointer, 30);
        BOOST_CHECK_EQUAL(c.get(2).size, 50);
    }

    {
        address c = a.range(80, 700);
        BOOST_CHECK_EQUAL(c.data_size(), 420);
        BOOST_CHECK_EQUAL(c.size(), 3);
        BOOST_CHECK_EQUAL(c.get(0).pointer, 80);
        BOOST_CHECK_EQUAL(c.get(0).size, 20);
        BOOST_CHECK_EQUAL(c.get(1).pointer, 30);
        BOOST_CHECK_EQUAL(c.get(1).size, 200);
        BOOST_CHECK_EQUAL(c.get(2).pointer, 30);
        BOOST_CHECK_EQUAL(c.get(2).size, 200);
    }
}

} // namespace uh::cluster
