#define BOOST_TEST_MODULE "ec tests"

#include <common/ec/reedsolomon_c.h>
#include <common/telemetry/log.h>
#include <common/types/common_types.h>
#include <common/utils/strings.h>
#include <common/utils/time_utils.h>
#include <util/random.h>

#include <boost/test/unit_test.hpp>

// ------------- Tests Suites Follow --------------

namespace uh::cluster {

BOOST_AUTO_TEST_SUITE(a_ec)

BOOST_AUTO_TEST_CASE(recovers_few_erasures) {
    const auto data_shards = 4ul;
    const auto parity_shards = 2ul;
    const auto chunk_size = 16_KiB;
    reedsolomon_c ec(data_shards, parity_shards, chunk_size);
    auto data_org = random_buffer(data_shards * chunk_size);
    auto data_org_view = split_buffer<char>(data_org, chunk_size);

    auto data = unique_buffer<char>(data_org.size());
    std::copy(data_org.begin(), data_org.end(), data.begin());

    auto parity = unique_buffer<char>{parity_shards * chunk_size};
    auto parity_view = split_buffer<char>(parity, chunk_size);

    auto stripe_view = split_buffer<char>(data, chunk_size);
    auto data_view = std::vector<std::span<const char>>();
    data_view.reserve(data_shards);
    for (const auto& dv : stripe_view) {
        // Dirty const_cast here is inevitable dirtiness to make real write
        // request handler cleaner, which gets vector<span<const char>> as it's
        // input.
        data_view.emplace_back(const_cast<const char*>(dv.data()), dv.size());
    }
    stripe_view.insert(stripe_view.end(), parity_view.begin(),
                       parity_view.end());

    ec.encode(data_view, parity_view);

    std::vector stats(data_shards + parity_shards, data_stat::valid);
    stats[1] = data_stat::lost;
    stats[3] = data_stat::lost;

    std::ranges::fill(stripe_view[1], 0);
    std::ranges::fill(stripe_view[3], 0);

    ec.recover(stripe_view, stats);

    BOOST_CHECK(std::ranges::equal(data_org_view[0], stripe_view[0]));
    BOOST_CHECK(std::ranges::equal(data_org_view[1], stripe_view[1]));
    BOOST_CHECK(std::ranges::equal(data_org_view[2], stripe_view[2]));
    BOOST_CHECK(std::ranges::equal(data_org_view[3], stripe_view[3]));
}

BOOST_AUTO_TEST_CASE(fails_to_recover_for_too_many_erasures) {
    const auto data_shards = 4ul;
    const auto parity_shards = 2ul;
    const auto chunk_size = 16_KiB;
    reedsolomon_c ec(data_shards, parity_shards, chunk_size);
    auto data_org = random_buffer(data_shards * chunk_size);
    auto data_org_view = split_buffer<char>(data_org, chunk_size);

    auto data = unique_buffer<char>(data_org.size());
    std::copy(data_org.begin(), data_org.end(), data.begin());

    auto parity = unique_buffer<char>{parity_shards * chunk_size};
    auto parity_view = split_buffer<char>(parity, chunk_size);

    auto stripe_view = split_buffer<char>(data, chunk_size);
    auto data_view = std::vector<std::span<const char>>();
    data_view.reserve(data_shards);
    for (const auto& dv : stripe_view) {
        // Dirty const_cast here is inevitable dirtiness to make real write
        // request handler cleaner, which gets vector<span<const char>> as it's
        // input.
        data_view.emplace_back(const_cast<const char*>(dv.data()), dv.size());
    }
    stripe_view.insert(stripe_view.end(), parity_view.begin(),
                       parity_view.end());

    ec.encode(data_view, parity_view);

    std::vector stats(data_shards + parity_shards, data_stat::valid);
    stats[1] = data_stat::lost;
    stats[3] = data_stat::lost;
    stats[4] = data_stat::lost;

    std::ranges::fill(stripe_view[1], 0);
    std::ranges::fill(stripe_view[3], 0);
    std::ranges::fill(stripe_view[4], 0);

    BOOST_CHECK_THROW(ec.recover(stripe_view, stats), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(recovers_multiple_times) {
    const auto data_shards = 4ul;
    const auto parity_shards = 2ul;
    const auto chunk_size = 16_KiB;
    reedsolomon_c ec(data_shards, parity_shards, chunk_size);

    for (auto i = 0ul; i < 10; ++i) {
        auto data_org = random_buffer(data_shards * chunk_size);
        auto data_org_view = split_buffer<char>(data_org, chunk_size);

        auto data = unique_buffer<char>(data_org.size());
        std::copy(data_org.begin(), data_org.end(), data.begin());

        auto parity = unique_buffer<char>{parity_shards * chunk_size};
        auto parity_view = split_buffer<char>(parity, chunk_size);

        auto stripe_view = split_buffer<char>(data, chunk_size);
        auto data_view = std::vector<std::span<const char>>();
        data_view.reserve(data_shards);
        for (const auto& dv : stripe_view) {
            // Dirty const_cast here is inevitable dirtiness to make real write
            // request handler cleaner, which gets vector<span<const char>> as
            // it's input.
            data_view.emplace_back(const_cast<const char*>(dv.data()),
                                   dv.size());
        }
        stripe_view.insert(stripe_view.end(), parity_view.begin(),
                           parity_view.end());

        ec.encode(data_view, parity_view);

        std::vector stats(data_shards + parity_shards, data_stat::valid);
        stats[1] = data_stat::lost;
        stats[3] = data_stat::lost;

        std::ranges::fill(stripe_view[1], 0);
        std::ranges::fill(stripe_view[3], 0);

        ec.recover(stripe_view, stats);

        BOOST_CHECK(std::ranges::equal(data_org_view[0], stripe_view[0]));
        BOOST_CHECK(std::ranges::equal(data_org_view[1], stripe_view[1]));
        BOOST_CHECK(std::ranges::equal(data_org_view[2], stripe_view[2]));
        BOOST_CHECK(std::ranges::equal(data_org_view[3], stripe_view[3]));
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_CASE(ec_zeros) {
    const auto data_shards = 3ul;
    const auto parity_shards = 2ul;
    const auto chunk_size = 4_KiB;
    reedsolomon_c ec(data_shards, parity_shards, chunk_size);
    unique_buffer<char> data(data_shards * chunk_size);
    auto data_view = split_buffer<const char>(data, chunk_size);
    auto parity = unique_buffer<char>{parity_shards * chunk_size};
    auto parity_view = split_buffer<char>(parity, chunk_size);

    std::ranges::fill(data, 0);
    ec.encode(data_view, parity_view);

    BOOST_CHECK(std::ranges::equal(data_view[0], data_view[1]));
    BOOST_CHECK(std::ranges::equal(data_view[1], data_view[2]));
    BOOST_CHECK(std::ranges::equal(data_view[2], parity_view[0]));
    BOOST_CHECK(std::ranges::equal(parity_view[0], parity_view[1]));
}

} // namespace uh::cluster
