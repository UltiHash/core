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

std::vector<unsigned char> binary_generator() {
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<std::mt19937::result_type> dist(16, STORE_MAX);

    std::size_t len = dist(rng);

    std::random_device dev2;
    std::mt19937 rng2(dev2());
    std::uniform_int_distribution<std::mt19937::result_type> dist2(0, UINT64_MAX);

    std::random_device dev3;
    std::mt19937 rng3(dev3());
    std::uniform_int_distribution<std::mt19937::result_type> dist3(0, UINT8_MAX);

    std::vector<unsigned char> out_return;
    out_return.reserve(len);
    std::size_t i = 0;
    for (; i < len - sizeof(std::size_t); i += sizeof(std::size_t)) {
        std::memcpy(reinterpret_cast<void *>(out_return[i]), reinterpret_cast<const std::size_t *>(dist2(rng2)),
                    sizeof(std::size_t));
    }

    for (; i < len; i++) {
        out_return[i] = dist3(rng3);
    }

    return out_return;
}

// ------------- Tests Follow --------------
BOOST_AUTO_TEST_CASE(constructor_test)
{
    uh::trees::tree_storage t1("/mnt/md0");//A test folder reserved for tree storage
}

BOOST_AUTO_TEST_CASE(write_read_test)
{
    std::size_t total_size{};
    uh::trees::tree_storage t1("/mnt/md0");//A test folder reserved for tree storage
    struct timeval time{};

    //list of write times with local_block_ref, integrated block size and milliseconds
    std::vector<std::tuple<std::vector<unsigned char>, std::size_t, long double>> write_times;
    //retrieved block size, local_block_ref size and time taken
    std::vector<std::tuple<std::size_t, std::size_t, long double>> read_after_write_times,linear_read,randam_access_read;

    while (total_size < (std::size_t)(std::pow(1024, 4) * 4)) {//write 4TB for testing
        std::vector<unsigned char> test_bin = binary_generator();
        //write test
        gettimeofday(&time, nullptr);
        long double millis = ((long double) time.tv_sec * 1000) + ((long double) time.tv_usec / 1000);
        std::vector<unsigned char> local_block_ref = t1.write(test_bin);
        gettimeofday(&time, nullptr);
        long double write_time = (((long double) time.tv_sec * 1000) + ((long double) time.tv_usec / 1000)) - millis;
        BOOST_CHECK_MESSAGE(!local_block_ref.empty(), std::string(
                "Database writing failed at block size " + std::to_string(test_bin.size()) + " at total size " +
                std::to_string(total_size) + " . No reference retrieved!").c_str());
        if(local_block_ref.empty())continue;
        write_times.emplace_back(local_block_ref,test_bin.size(),write_time);
        //read after write test
        gettimeofday(&time, nullptr);
        millis = ((long double) time.tv_sec * 1000) + ((long double) time.tv_usec / 1000);
        std::vector<unsigned char> read_result = t1.read(local_block_ref);
        gettimeofday(&time, nullptr);
        long double read_after_write_time =
                (((long double) time.tv_sec * 1000) + ((long double) time.tv_usec / 1000)) - millis;
        //check correctness of stored string
        BOOST_CHECK_EQUAL_COLLECTIONS(test_bin.cbegin(), test_bin.cend(), read_result.cbegin(), read_result.cend());

        read_after_write_times.emplace_back(read_result.size(), local_block_ref.size(), read_after_write_time);

        total_size += test_bin.size();
    }

    BOOST_CHECK_MESSAGE(!write_times.empty(),"No write times were tested!");
    //test sequential read from beginning on
    for(const auto &i:write_times){
        gettimeofday(&time, nullptr);
        long double millis = ((long double) time.tv_sec * 1000) + ((long double) time.tv_usec / 1000);
        std::vector<unsigned char> read_result = t1.read(std::get<0>(i));
        gettimeofday(&time, nullptr);
        long double read_sequential =
                (((long double) time.tv_sec * 1000) + ((long double) time.tv_usec / 1000)) - millis;
        BOOST_CHECK_MESSAGE(!read_result.empty(), std::string(
                "Database sequential reading failed at block reference " +
                boost::algorithm::hex(std::string{std::get<0>(i).cbegin(),std::get<0>(i).cend()}) + " . No block retrieved!").c_str());
        linear_read.emplace_back(read_result.size(),std::get<0>(i).size(),read_sequential);
    }

    //test random access times for 256GB
    total_size = 0;
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<std::mt19937::result_type> dist(0, write_times.size());
    while(total_size<(std::size_t)std::pow(2,38)){
        std::size_t access_point = dist(rng);
        gettimeofday(&time, nullptr);
        long double millis = ((long double) time.tv_sec * 1000) + ((long double) time.tv_usec / 1000);
        std::vector<unsigned char> read_result = t1.read(std::get<0>(write_times[access_point]));
        gettimeofday(&time, nullptr);
        long double read_sequential =
                (((long double) time.tv_sec * 1000) + ((long double) time.tv_usec / 1000)) - millis;
        BOOST_CHECK_MESSAGE(!read_result.empty(), std::string(
                "Database random access reading failed at block reference " +
                boost::algorithm::hex(std::string{std::get<0>(write_times[access_point]).cbegin(),std::get<0>(write_times[access_point]).cend()}) + " . No block retrieved!").c_str());
        randam_access_read.emplace_back(read_result.size(),std::get<0>(write_times[access_point]).size(),read_sequential);
        total_size+=read_result.size();
    }


}