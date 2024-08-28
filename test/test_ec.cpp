#define BOOST_TEST_MODULE "ec tests"

#include "common/types/common_types.h"

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
    std::memset(const_cast<char*>(shards.at(1).data()), 0, shards.at(1).size());
    std::memset(const_cast<char*>(shards.at(3).data()), 0, shards.at(3).size());
    std::vector<ec_calculator::data_stat> stats(
        6, ec_calculator::data_stat::valid);
    stats[1] = ec_calculator::data_stat::lost;
    stats[3] = ec_calculator::data_stat::lost;

    BOOST_CHECK(shards.at(1) != encoded.get().at(1));
    BOOST_CHECK(shards.at(3) != encoded.get().at(3));
    ec.recover(shards, stats);

    int i = 0;
    for (auto s : shards) {
        BOOST_CHECK(s == encoded.get().at(i++));
    }
}

} // namespace uh::cluster
