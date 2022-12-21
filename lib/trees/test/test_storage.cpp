//
// Created by benjamin on 18.12.22.
//

#define BOOST_TEST_DYN_LINK
#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "uhLibTrees Tree Storage Tests"
#endif

#include <boost/test/unit_test.hpp>
#include <trees/tree_storage.h>
#include <random>

std::vector<unsigned char> binary_generator(){
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<std::mt19937::result_type> dist(16,STORE_MAX);

    std::size_t len = dist(rng);

    std::random_device dev2;
    std::mt19937 rng2(dev2());
    std::uniform_int_distribution<std::mt19937::result_type> dist2(0,UINT8_MAX);
    std::vector<unsigned char> rands;
    rands.reserve(len);
    for(std::size_t i = 0; i<len;i++){
        rands.push_back(dist2(rng2));
    }
    return rands;
}

// ------------- Tests Follow --------------
BOOST_AUTO_TEST_CASE( constructor1 )
{
    uh::trees::tree_storage t1("/mnt/md0");//A test folder reserved for tree storage
}