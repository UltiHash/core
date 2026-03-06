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

#define BOOST_TEST_MODULE "deduplicator tests"

#include <boost/test/unit_test.hpp>

#include <common/utils/random.h>
#include <util/dedupe_fixture.h>

using namespace uh::cluster;

BOOST_FIXTURE_TEST_CASE(deduplicate, dedupe_fixture) {

    auto data = random_string(66);

    auto f = [&]() -> coro<dedupe_response> {
        co_return co_await dedupe.deduplicate(data);
    };
    {
        std::future<dedupe_response> res = spawn(f);
        auto dedup_response = res.get();
        BOOST_TEST(dedup_response.effective_size == data.size());
        BOOST_TEST(data_store.get_used_space() == data.size());
    }
    {
        std::future<dedupe_response> res = spawn(f);
        auto dedup_response = res.get();
        BOOST_TEST(dedup_response.effective_size == 0);
        BOOST_TEST(data_store.get_used_space() == data.size());
    }
    {
        std::future<dedupe_response> res = spawn(f);
        auto dedup_response = res.get();
        BOOST_TEST(dedup_response.effective_size == 0);
        BOOST_TEST(data_store.get_used_space() == data.size());
    }
}
