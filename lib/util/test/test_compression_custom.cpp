//
// Created by benjamin-elias on 20.01.23.
//

#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "uhLibUtil Compression Tests"
#endif

#include <boost/test/unit_test.hpp>
#include <util/compression_custom.h>
#include <random>
#include <trees/tree_storage_config.h>
#include <trees/tree_storage_config.h.in>

std::vector<unsigned char> binary_generator(std::size_t max_len) {
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<std::mt19937::result_type> dist(1, max_len);

    std::size_t len = dist(rng);

    std::random_device dev3;
    std::mt19937 rng3(dev3());
    std::uniform_int_distribution<std::mt19937::result_type> dist3(0, UINT8_MAX);

    std::vector<unsigned char> out_return;
    out_return.resize(len);
    std::for_each(out_return.begin(), out_return.end(), [&dist3, &rng3](auto &a) {
        a = dist3(rng3);
    });

    return out_return;
}

// ------------- Tests Follow --------------
BOOST_AUTO_TEST_CASE( compress_equal )
{
    uh::util::compression_custom c;
    for(int counter=0;counter<10;counter++){
        auto to_check = binary_generator(STORE_MAX);
        auto compressed = c.compress(to_check);
        auto decompressed = c.decompress(compressed);
        BOOST_CHECK_EQUAL_COLLECTIONS(to_check.begin(),to_check.end(),decompressed.begin(),decompressed.end());
    }

}
