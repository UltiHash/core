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
#include <boost/test/data/test_case.hpp>
#include <boost/test/data/monomorphic.hpp>
#include <trees/tree_storage.h>
#include <random>

std::vector<unsigned char> binary_generator(){
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<std::mt19937::result_type> dist(16,STORE_MAX);

    std::size_t len = dist(rng);

    std::random_device dev2;
    std::mt19937 rng2(dev2());
    std::uniform_int_distribution<std::mt19937::result_type> dist2(0,UINT64_MAX);

    std::random_device dev3;
    std::mt19937 rng3(dev3());
    std::uniform_int_distribution<std::mt19937::result_type> dist3(0,UINT8_MAX);

    auto* out_ptr = (unsigned char*) std::malloc(sizeof(unsigned char)*len);
    std::size_t i = 0;
    for(; i<len-sizeof(std::size_t);i+=sizeof(std::size_t)){
        std::memcpy(reinterpret_cast<void *>(out_ptr[i]), reinterpret_cast<const std::size_t *>(dist2(rng2)), sizeof(std::size_t));
    }

    for(;i<len;i++){
        out_ptr[i] = dist3(rng3);
    }

    std::vector<unsigned char> out_return{out_ptr,out_ptr+len};

    return out_return;
}

// ------------- Tests Follow --------------
BOOST_AUTO_TEST_CASE( constructor_test )
{
    uh::trees::tree_storage t1("/mnt/md0");//A test folder reserved for tree storage
}

