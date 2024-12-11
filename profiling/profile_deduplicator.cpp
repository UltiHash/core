#include "common/test/coroutine.h"
#include "common/utils/common.h"
#include "common/utils/temp_directory.h"
#include "deduplicator/interfaces/local_deduplicator.h"
#include "fakes/storage/fake_global_data_view.h"
#include "utils/random_string.h"

#include <benchmark/benchmark.h>
#include <boost/asio.hpp>
#include <memory>

namespace uh::cluster {

#define MAX_FILE_SIZE_BYTES (128 * KIBI_BYTE)
#define DATA_STORE_ID 1

struct deduplicator_benchmark : public benchmark::Fixture, coro_fixture {
public:
    void SetUp(const ::benchmark::State& state) override {
        input_data = generate_random_string(state.range(0));

        auto log_config = log::config{
            .sinks = {log::sink_config{.type = log::sink_type::cout,
                                       .level = boost::log::trivial::fatal,
                                       .service_role = DEDUPLICATOR_SERVICE}}};
        log::init(log_config);

        auto config =
            data_store_config{.max_file_size = MAX_FILE_SIZE_BYTES,
                              .max_data_store_size =
                                  (size_t)state.max_iterations * state.range(0),
                              .page_size = DEFAULT_PAGE_SIZE};
        data_store = std::make_unique<fake_data_store>(
            config, dir.path().string(), DATA_STORE_ID, 0);
        data_view = std::make_unique<fake_global_data_view>(get_io_context(),
                                                            *data_store);

        dedup = std::make_unique<local_deduplicator>(deduplicator_config{},
                                                     *data_view);
    }

    void TearDown(const ::benchmark::State& state) override {}

    deduplicator_benchmark()
        : benchmark::Fixture(),
          coro_fixture{1} {}

protected:
    std::string input_data;
    temp_directory dir;
    std::unique_ptr<fake_data_store> data_store;
    std::unique_ptr<fake_global_data_view> data_view;
    std::unique_ptr<local_deduplicator> dedup;
    context ctx;
};

BENCHMARK_DEFINE_F(deduplicator_benchmark, profile_dedup_with_random_input)
(benchmark::State& state) {

    for (auto _ : state) {
        auto f = [this]() -> coro<dedupe_response> {
            co_return co_await dedup->deduplicate(ctx, input_data);
        };

        std::future<dedupe_response> res = spawn(f);
        auto dedup_response = res.get();
        benchmark::DoNotOptimize(dedup_response);
        // data_store->clear();
    }
}

BENCHMARK_REGISTER_F(deduplicator_benchmark, profile_dedup_with_random_input)
    ->Iterations(50000)
    ->Arg(DEFAULT_PAGE_SIZE);

} // namespace uh::cluster
