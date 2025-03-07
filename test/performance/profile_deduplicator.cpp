#include "common/utils/common.h"
#include "common/utils/random.h"
#include "deduplicator/interfaces/local_deduplicator.h"
#include "lib/mock/storage/mock_global_data_view.h"
#include "lib/util/coroutine.h"
#include <lib/util/temp_directory.h>

#include <benchmark/benchmark.h>
#include <boost/asio.hpp>
#include <memory>

namespace uh::cluster {

#define MAX_FILE_SIZE_BYTES (128 * KIBI_BYTE)
#define DATA_STORE_ID 1

struct deduplicator_benchmark : public benchmark::Fixture, coro_fixture {
public:
    void SetUp(const ::benchmark::State& state) override {
        auto log_config = log::config{
            .sinks = {log::sink_config{.type = log::sink_type::cout,
                                       .level = boost::log::trivial::fatal,
                                       .service_role = DEDUPLICATOR_SERVICE}}};
        log::init(log_config);

        auto config = data_store_config{.max_file_size = MAX_FILE_SIZE_BYTES,
                                        .max_data_store_size =
                                            (size_t)state.max_iterations *
                                            state.range(0) * 2,
                                        .page_size = DEFAULT_PAGE_SIZE};
        data_store = std::make_unique<mock_data_store>(
            config, dir.path().string(), DATA_STORE_ID, 0);
        data_view = std::make_unique<mock_global_data_view>(*data_store);

        dedup = std::make_unique<local_deduplicator>(deduplicator_config{},
                                                     *data_view);
    }

    void TearDown(const ::benchmark::State& state) override {}

    deduplicator_benchmark()
        : benchmark::Fixture(),
          coro_fixture{2} {}

protected:
    temp_directory dir;
    std::unique_ptr<mock_data_store> data_store;
    std::unique_ptr<mock_global_data_view> data_view;
    std::unique_ptr<local_deduplicator> dedup;
    context ctx;
};

BENCHMARK_DEFINE_F(deduplicator_benchmark, profile_dedup_with_same_data)
(benchmark::State& state) {

    std::string input_data = random_string(state.range(0));
    auto f = [&]() -> coro<dedupe_response> {
        co_return co_await dedup->deduplicate(ctx, input_data);
    };

    std::future<dedupe_response> res = spawn(f);
    auto dedup_response = res.get();
    benchmark::DoNotOptimize(dedup_response);

    for (auto _ : state) {
        auto f = [&]() -> coro<dedupe_response> {
            co_return co_await dedup->deduplicate(ctx, input_data);
        };

        std::future<dedupe_response> res = spawn(f);
        auto dedup_response = res.get();
        benchmark::DoNotOptimize(dedup_response);
    }
}

BENCHMARK_REGISTER_F(deduplicator_benchmark, profile_dedup_with_same_data)
    ->Iterations(50000)
    ->Arg(DEFAULT_PAGE_SIZE / 2);

} // namespace uh::cluster
