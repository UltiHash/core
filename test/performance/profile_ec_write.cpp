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

#include <common/utils/common.h>
#include <common/utils/random.h>

#define WITH_EC
#include <util/gdv_fixture.h>

#include <mock/storage/mock_data_view.h>
#include <util/coroutine.h>
#include <util/random.h>
#include <util/temp_directory.h>

#include <benchmark/benchmark.h>
#include <boost/asio.hpp>
#include <memory>

namespace uh::cluster {

#define MAX_FILE_SIZE_BYTES (128 * KIBI_BYTE)
#define DATA_STORE_ID 1

struct deduplicator_benchmark : public benchmark::Fixture,
                                global_data_view_fixture {
    deduplicator_benchmark()
        : benchmark::Fixture(),
          global_data_view_fixture({
              .id = 0,
              .type = storage::group_config::type_t::ERASURE_CODING,
              .storages = 6,
              .data_shards = 4,
              .parity_shards = 2,
              .stripe_size_kib = 4 * 2,
          }),
          buffer{
              random_buffer(get_group_config().stripe_size_kib * 1_KiB * 7)} {}

    ~deduplicator_benchmark() {}

    void SetUp(const ::benchmark::State& state) override { setup(); }
    void TearDown(const ::benchmark::State& state) override { teardown(); }
    shared_buffer<char> buffer;
};

BENCHMARK_DEFINE_F(deduplicator_benchmark, profile_dedup_with_same_data)
(benchmark::State& state) {
    auto gdv = get_data_view();
    auto& ioc = get_executor();

    for (auto _ : state) {
        boost::asio::co_spawn(ioc, gdv->write(buffer.string_view(), {0}),
                              boost::asio::use_future)
            .get();
    }
}

BENCHMARK_REGISTER_F(deduplicator_benchmark, profile_dedup_with_same_data)
    ->Iterations(50)
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime()
    ->MinTime(1.0);

} // namespace uh::cluster
