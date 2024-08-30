#define BOOST_TEST_MODULE "ec tests"

#include "common/telemetry/log.h"
#include "common/types/common_types.h"
#include "common/utils/time_utils.h"

#include <boost/test/unit_test.hpp>
#include <common/ec/ec_calculator.h>

// ------------- Tests Suites Follow --------------

namespace uh::cluster {

void fill_random(char* buf, size_t size) {
    for (size_t i = 0; i < size; ++i) {
        buf[i] = rand() & 0xff;
    }
}

BOOST_AUTO_TEST_CASE(ec_basic) {
    ec_calculator ec(4, 2);
    std::string data(64 * KIBI_BYTE, '0');
    fill_random(data.data(), data.size());
    auto encoded = ec.encode(data);
    auto shards = encoded.get();
    std::vector stats(6, ec_calculator::data_stat::valid);
    stats[1] = ec_calculator::data_stat::lost;
    stats[3] = ec_calculator::data_stat::lost;

    std::string s1(shards.at(1).size(), '0');
    std::string s3(shards.at(1).size(), '0');
    shards.at(1) = s1;
    shards.at(3) = s3;

    BOOST_CHECK(shards.at(1) != encoded.get().at(1));
    BOOST_CHECK(shards.at(3) != encoded.get().at(3));
    ec.recover(shards, stats);

    int i = 0;
    for (auto s : shards) {
        BOOST_CHECK(s == encoded.get().at(i++));
    }
}

BOOST_AUTO_TEST_CASE(ec_basic_lost) {
    ec_calculator ec(4, 2);
    std::string data(64 * KIBI_BYTE, '0');
    fill_random(data.data(), data.size());
    auto encoded = ec.encode(data);
    auto shards = encoded.get();
    std::vector stats(6, ec_calculator::data_stat::valid);
    stats[1] = ec_calculator::data_stat::lost;
    stats[3] = ec_calculator::data_stat::lost;
    stats[4] = ec_calculator::data_stat::lost;

    std::string s1(shards.at(1).size(), '0');
    std::string s3(shards.at(1).size(), '0');
    std::string s4(shards.at(1).size(), '0');

    shards.at(1) = s1;
    shards.at(3) = s3;
    shards.at(4) = s4;

    BOOST_CHECK(shards.at(1) != encoded.get().at(1));
    BOOST_CHECK(shards.at(3) != encoded.get().at(3));
    BOOST_CHECK(shards.at(4) != encoded.get().at(4));

    BOOST_CHECK_THROW(ec.recover(shards, stats), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(no_ec) {
    ec_calculator ec(4, 0);
    std::string data(64 * KIBI_BYTE, '0');
    fill_random(data.data(), data.size());
    auto encoded = ec.encode(data);
    auto shards = encoded.get();
    const auto shard_size = shards.front().size();
    size_t i = 0;
    for (auto s : shards) {
        BOOST_CHECK(s == data.substr(i * shard_size,
                                     std::min(shard_size,
                                              data.size() - i * shard_size)));
        i++;
    }
}

BOOST_AUTO_TEST_CASE(ec_non_divisable) {
    ec_calculator ec(7, 3);
    std::string data(64 * KIBI_BYTE, '0');
    fill_random(data.data(), data.size());
    auto encoded = ec.encode(data);
    auto shards = encoded.get();
    std::vector stats(7, ec_calculator::data_stat::valid);
    stats[1] = ec_calculator::data_stat::lost;
    stats[3] = ec_calculator::data_stat::lost;
    stats[4] = ec_calculator::data_stat::lost;

    std::string s1(shards.at(1).size(), '0');
    std::string s3(shards.at(1).size(), '0');
    std::string s4(shards.at(1).size(), '0');

    shards.at(1) = s1;
    shards.at(3) = s3;
    shards.at(4) = s4;

    BOOST_CHECK(shards.at(1) != encoded.get().at(1));
    BOOST_CHECK(shards.at(3) != encoded.get().at(3));
    BOOST_CHECK(shards.at(4) != encoded.get().at(4));

    ec.recover(shards, stats);

    int i = 0;
    for (auto s : shards) {
        BOOST_CHECK(s == encoded.get().at(i++));
    }
}

BOOST_AUTO_TEST_CASE(large_data) {
    ec_calculator ec(8, 4);
    std::string data(1 * GIBI_BYTE, '0');
    fill_random(data.data(), data.size());

    auto encoded = ec.encode(data);

    auto shards = encoded.get();
    std::vector stats(12, ec_calculator::data_stat::valid);
    stats[1] = ec_calculator::data_stat::lost;
    stats[3] = ec_calculator::data_stat::lost;
    stats[4] = ec_calculator::data_stat::lost;

    std::string s1(shards.at(1).size(), '0');
    std::string s3(shards.at(1).size(), '0');
    std::string s4(shards.at(1).size(), '0');

    shards.at(1) = s1;
    shards.at(3) = s3;
    shards.at(4) = s4;

    BOOST_CHECK(shards.at(1) != encoded.get().at(1));
    BOOST_CHECK(shards.at(3) != encoded.get().at(3));
    BOOST_CHECK(shards.at(4) != encoded.get().at(4));

    ec.recover(shards, stats);

    int i = 0;
    for (auto s : shards) {
        BOOST_CHECK(s == encoded.get().at(i++));
    }
}

} // namespace uh::cluster
