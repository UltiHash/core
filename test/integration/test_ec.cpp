#define BOOST_TEST_MODULE "ec tests"

#include <common/ec/reedsolomon_c.h>
#include <common/telemetry/log.h>
#include <common/types/common_types.h>
#include <common/utils/time_utils.h>
#include <util/random.h>

#include <boost/test/unit_test.hpp>

// ------------- Tests Suites Follow --------------

namespace uh::cluster {

struct EncodedData {
    unique_buffer<char> data;
    std::vector<unique_buffer<char>> parity;
    std::vector<std::span<char>> shards;
};

EncodedData
copy_encoded(std::size_t data_shards,
             const std::vector<std::span<const char>>& input_shards) {
    auto chunk_size = input_shards.at(0).size();

    unique_buffer<char> data(data_shards * input_shards.at(0).size());
    std::vector<unique_buffer<char>> parity;
    for (auto i = 0ul; i < input_shards.size() - data_shards; ++i) {
        parity.emplace_back(chunk_size);
    }
    std::vector<std::span<char>> new_shards;
    new_shards.reserve(input_shards.size());
    for (size_t i = 0; i < data_shards; ++i) {
        new_shards.emplace_back(data.data() + i * chunk_size, chunk_size);
    }
    for (size_t i = 0; i < input_shards.size() - data_shards; ++i) {
        new_shards.emplace_back(parity[i].data(), chunk_size);
    }
    for (size_t i = 0; i < input_shards.size(); ++i) {
        std::ranges::copy(input_shards.at(i), new_shards.at(i).begin());
    }
    return {std::move(data), std::move(parity), std::move(new_shards)};
}

BOOST_AUTO_TEST_CASE(ec_basic) {
    const auto data_shards = 4ul;
    const auto parity_shards = 2ul;
    const auto chunk_size = 16_KiB;
    reedsolomon_c ec(data_shards, parity_shards, chunk_size);
    auto data = random_buffer(data_shards * chunk_size);
    auto encoded = ec.encode(data);
    auto shards = encoded.get();

    EncodedData new_enc = copy_encoded(data_shards, shards);

    std::vector stats(data_shards + parity_shards, data_stat::valid);
    stats[1] = data_stat::lost;
    stats[3] = data_stat::lost;

    std::ranges::fill(new_enc.shards[1], 0);
    std::ranges::fill(new_enc.shards[3], 0);

    ec.recover(new_enc.shards, stats);

    BOOST_CHECK(std::ranges::equal(shards[0], new_enc.shards[0]));
    BOOST_CHECK(std::ranges::equal(shards[1], new_enc.shards[1]));
    BOOST_CHECK(std::ranges::equal(shards[2], new_enc.shards[2]));
    BOOST_CHECK(std::ranges::equal(shards[3], new_enc.shards[3]));
    BOOST_CHECK(std::ranges::equal(shards[4], new_enc.shards[4]));
    BOOST_CHECK(std::ranges::equal(shards[5], new_enc.shards[5]));
}

BOOST_AUTO_TEST_CASE(ec_basic_lost) {

    const auto data_shards = 4ul;
    const auto parity_shards = 2ul;
    const auto chunk_size = 16_KiB;
    reedsolomon_c ec(data_shards, parity_shards, chunk_size);
    auto data = random_buffer(data_shards * chunk_size);
    auto encoded = ec.encode(data);
    auto shards = encoded.get();

    EncodedData new_enc = copy_encoded(data_shards, shards);

    std::vector stats(data_shards + parity_shards, data_stat::valid);
    stats[1] = data_stat::lost;
    stats[3] = data_stat::lost;
    stats[4] = data_stat::lost;

    std::ranges::fill(new_enc.shards[1], 0);
    std::ranges::fill(new_enc.shards[3], 0);
    std::ranges::fill(new_enc.shards[4], 0);

    BOOST_CHECK_THROW(ec.recover(new_enc.shards, stats), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(two_times_ec) {
    const auto data_shards = 4ul;
    const auto parity_shards = 2ul;
    const auto chunk_size = 16;
    reedsolomon_c ec(data_shards, parity_shards, chunk_size);

    for (auto i = 0ul; i < 2; ++i) {
        auto data = random_buffer(data_shards * chunk_size);
        auto encoded = ec.encode(data);
        auto shards = encoded.get();

        EncodedData new_enc = copy_encoded(data_shards, shards);

        std::vector stats(data_shards + parity_shards, data_stat::valid);
        stats[1] = data_stat::lost;
        stats[3] = data_stat::lost;

        std::ranges::fill(new_enc.shards[1], 0);
        std::ranges::fill(new_enc.shards[3], 0);

        ec.recover(new_enc.shards, stats);

        BOOST_CHECK(std::ranges::equal(shards[1], new_enc.shards[1]));
        BOOST_CHECK(std::ranges::equal(shards[3], new_enc.shards[3]));
    }
}

} // namespace uh::cluster
