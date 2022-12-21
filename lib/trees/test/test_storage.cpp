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
    std::vector<std::tuple<std::vector<unsigned char>, std::size_t, long double>> write_times,read_after_write_times, linear_read, randam_access_read;
    //retrieved block size, local_block_ref size and time taken

    while (total_size < (std::size_t) (std::pow(1024, 4) * 4)) {//write 4TB for testing
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
        if (local_block_ref.empty())continue;
        write_times.emplace_back(local_block_ref, test_bin.size(), write_time);
        //read after write test
        gettimeofday(&time, nullptr);
        millis = ((long double) time.tv_sec * 1000) + ((long double) time.tv_usec / 1000);
        std::vector<unsigned char> read_result = t1.read(local_block_ref);
        gettimeofday(&time, nullptr);
        long double read_after_write_time =
                (((long double) time.tv_sec * 1000) + ((long double) time.tv_usec / 1000)) - millis;
        //check correctness of stored string
        BOOST_CHECK_EQUAL_COLLECTIONS(test_bin.cbegin(), test_bin.cend(), read_result.cbegin(), read_result.cend());

        read_after_write_times.emplace_back(local_block_ref,read_result.size(), read_after_write_time);

        total_size += test_bin.size();
    }

    BOOST_CHECK_MESSAGE(!write_times.empty(), "No write times were tested!");
    //test sequential read from beginning on
    for (const auto &i: write_times) {
        gettimeofday(&time, nullptr);
        long double millis = ((long double) time.tv_sec * 1000) + ((long double) time.tv_usec / 1000);
        std::vector<unsigned char> read_result = t1.read(std::get<0>(i));
        gettimeofday(&time, nullptr);
        long double read_sequential =
                (((long double) time.tv_sec * 1000) + ((long double) time.tv_usec / 1000)) - millis;
        BOOST_CHECK_MESSAGE(!read_result.empty(), std::string(
                "Database sequential reading failed at block reference " +
                boost::algorithm::hex(std::string{std::get<0>(i).cbegin(), std::get<0>(i).cend()}) +
                " . No block retrieved!").c_str());
        linear_read.emplace_back(std::get<0>(i),read_result.size(), read_sequential);
    }

    //test random access times for 256GB
    total_size = 0;
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<std::mt19937::result_type> dist(0, write_times.size());
    while (total_size < (std::size_t) (std::pow(2, 38))) {
        std::size_t access_point = dist(rng);
        gettimeofday(&time, nullptr);
        long double millis = ((long double) time.tv_sec * 1000) + ((long double) time.tv_usec / 1000);
        std::vector<unsigned char> read_result = t1.read(std::get<0>(write_times[access_point]));
        gettimeofday(&time, nullptr);
        long double read_sequential =
                (((long double) time.tv_sec * 1000) + ((long double) time.tv_usec / 1000)) - millis;
        BOOST_CHECK_MESSAGE(!read_result.empty(), std::string(
                "Database random access reading failed at block reference " +
                boost::algorithm::hex(std::string{std::get<0>(write_times[access_point]).cbegin(),
                                                  std::get<0>(write_times[access_point]).cend()}) +
                " . No block retrieved!").c_str());
        randam_access_read.emplace_back(std::get<0>(write_times[access_point]), read_result.size(),
                                        read_sequential);
        total_size += read_result.size();
    }

    //write times min, max, average on size/block ref size/time taken
    BOOST_TEST_MESSAGE("Test results for writing:\n");
    BOOST_TEST_MESSAGE("Minimum results:");
    //minimum size
    auto write_min_size = std::min_element(write_times.cbegin(), write_times.cend(),
                                     [](const auto &a, const auto &b) { return std::get<1>(a) < std::get<1>(b); });
    BOOST_TEST_MESSAGE("Minimum size is " + std::to_string(std::get<1>(*write_min_size)) +
                       " from Block reference \"" + boost::algorithm::hex(
            std::string{std::get<0>(*write_min_size).cbegin(), std::get<0>(*write_min_size).cend()}) + "\" with a size of " +
                       std::to_string(std::get<0>(*write_min_size).size()) + " with an integration time of " +
                       std::to_string(std::get<2>(*write_min_size)) + " ms");
    //minimum block ref size
    auto write_min_block_ref_size = std::min_element(write_times.cbegin(), write_times.cend(),
                                               [](const auto &a, const auto &b) {
                                                   return std::get<0>(a).size() < std::get<0>(b).size();
                                               });
    BOOST_TEST_MESSAGE("Minimum block reference size is " + std::to_string(std::get<0>(*write_min_block_ref_size).size()) +
                       " from Block reference \"" + boost::algorithm::hex(
            std::string{std::get<0>(*write_min_block_ref_size).cbegin(), std::get<0>(*write_min_block_ref_size).cend()}) +
                       "\" with a total block size of " + std::to_string(std::get<1>(*write_min_block_ref_size)) +
                       " with an integration time of " +
                       std::to_string(std::get<2>(*write_min_block_ref_size)) + " ms");
    //minimum time taken
    auto write_min_time_taken = std::min_element(write_times.cbegin(), write_times.cend(), [](const auto &a, const auto &b) {
        return std::get<2>(a) < std::get<2>(b);
    });
    BOOST_TEST_MESSAGE("Minimum integration time is " + std::to_string(std::get<2>(*write_min_time_taken)) +
                       " ms from Block reference \"" + boost::algorithm::hex(
            std::string{std::get<0>(*write_min_time_taken).cbegin(), std::get<0>(*write_min_time_taken).cend()}) +
                       "\" with a block reference size of " +
                       std::to_string(std::get<0>(*write_min_time_taken).size()) + " with a total block size of " +
                       std::to_string(std::get<1>(*write_min_time_taken)) + "\n");

    BOOST_TEST_MESSAGE("Maximum results:");
    //maximum size
    auto write_max_size = std::max_element(write_times.cbegin(), write_times.cend(),
                                     [](const auto &a, const auto &b) { return std::get<1>(a) < std::get<1>(b); });
    BOOST_TEST_MESSAGE("Maximum size is " + std::to_string(std::get<1>(*write_max_size)) +
                       " from Block reference \"" + boost::algorithm::hex(
            std::string{std::get<0>(*write_max_size).cbegin(), std::get<0>(*write_max_size).cend()}) + "\" with a size of " +
                       std::to_string(std::get<0>(*write_max_size).size()) + " with an integration time of " +
                       std::to_string(std::get<2>(*write_max_size)) + " ms");
    //maximum block ref size
    auto write_max_block_ref_size = std::max_element(write_times.cbegin(), write_times.cend(),
                                               [](const auto &a, const auto &b) {
                                                   return std::get<0>(a).size() < std::get<0>(b).size();
                                               });
    BOOST_TEST_MESSAGE("Maximum block reference size is " + std::to_string(std::get<0>(*write_max_block_ref_size).size()) +
                       " from Block reference \"" + boost::algorithm::hex(
            std::string{std::get<0>(*write_max_block_ref_size).cbegin(), std::get<0>(*write_max_block_ref_size).cend()}) +
                       "\" with a total block size of " + std::to_string(std::get<1>(*write_max_block_ref_size)) +
                       " with an integration time of " +
                       std::to_string(std::get<2>(*write_max_block_ref_size)) + " ms");
    //maximum time taken
    auto write_max_time_taken = std::max_element(write_times.cbegin(), write_times.cend(), [](const auto &a, const auto &b) {
        return std::get<2>(a) < std::get<2>(b);
    });
    BOOST_TEST_MESSAGE("Maximum integration time is " + std::to_string(std::get<2>(*write_max_time_taken)) +
                       " ms from Block reference \"" + boost::algorithm::hex(
            std::string{std::get<0>(*write_max_time_taken).cbegin(), std::get<0>(*write_max_time_taken).cend()}) +
                       "\" with a block reference size of " +
                       std::to_string(std::get<0>(*write_max_time_taken).size()) + " with a total block size of " +
                       std::to_string(std::get<1>(*write_max_time_taken)) + "\n");

    long double write_avg_size = 0;
    long double write_avg_block_ref_size = 0;
    long double write_avg_time = 0;
    for(const auto &i:write_times){
        write_avg_size+=std::get<1>(i);
        write_avg_size+=std::get<0>(i).size();
        write_avg_time+=std::get<2>(i);
    }
    write_avg_size/=write_times.size();
    write_avg_block_ref_size/=write_times.size();
    write_avg_time/=write_times.size();

    long double write_integration_speed_MB = (write_avg_size/std::pow(2,20))/(write_avg_time/1000);

    BOOST_TEST_MESSAGE("Average writing results:");
    BOOST_TEST_MESSAGE("Average integration time is " + std::to_string(write_avg_time) +
                       " ms with an average block reference size of " +
                       std::to_string(write_avg_block_ref_size) + " with an average total block size of " +
                       std::to_string(write_avg_size) + ". This results in an average integration speed of "+std::to_string(write_integration_speed_MB)+" MB per second\n");

    //show read after write results
    BOOST_TEST_MESSAGE("Test results for read after write:\n");
    BOOST_TEST_MESSAGE("Minimum results:");
    //minimum size
    auto read_after_write_min_size = std::min_element(read_after_write_times.cbegin(), read_after_write_times.cend(),
                                           [](const auto &a, const auto &b) { return std::get<1>(a) < std::get<1>(b); });
    BOOST_TEST_MESSAGE("Minimum size is " + std::to_string(std::get<1>(*read_after_write_min_size)) +
                       " from Block reference \"" + boost::algorithm::hex(
            std::string{std::get<0>(*read_after_write_min_size).cbegin(), std::get<0>(*read_after_write_min_size).cend()}) + "\" with a size of " +
                       std::to_string(std::get<0>(*read_after_write_min_size).size()) + " with an read after write time of " +
                       std::to_string(std::get<2>(*read_after_write_min_size)) + " ms");
    //minimum block ref size
    auto read_after_write_min_block_ref_size = std::min_element(read_after_write_times.cbegin(), read_after_write_times.cend(),
                                                     [](const auto &a, const auto &b) {
                                                         return std::get<0>(a).size() < std::get<0>(b).size();
                                                     });
    BOOST_TEST_MESSAGE("Minimum block reference size is " + std::to_string(std::get<0>(*read_after_write_min_block_ref_size).size()) +
                       " from Block reference \"" + boost::algorithm::hex(
            std::string{std::get<0>(*read_after_write_min_block_ref_size).cbegin(), std::get<0>(*read_after_write_min_block_ref_size).cend()}) +
                       "\" with a total block size of " + std::to_string(std::get<1>(*read_after_write_min_block_ref_size)) +
                       " with an read after write time of " +
                       std::to_string(std::get<2>(*read_after_write_min_block_ref_size)) + " ms");
    //minimum time taken
    auto read_after_write_min_time_taken = std::min_element(read_after_write_times.cbegin(), read_after_write_times.cend(), [](const auto &a, const auto &b) {
        return std::get<2>(a) < std::get<2>(b);
    });
    BOOST_TEST_MESSAGE("Minimum read after write time is " + std::to_string(std::get<2>(*read_after_write_min_time_taken)) +
                       " ms from Block reference \"" + boost::algorithm::hex(
            std::string{std::get<0>(*read_after_write_min_time_taken).cbegin(), std::get<0>(*read_after_write_min_time_taken).cend()}) +
                       "\" with a block reference size of " +
                       std::to_string(std::get<0>(*read_after_write_min_time_taken).size()) + " with a total block size of " +
                       std::to_string(std::get<1>(*read_after_write_min_time_taken)) + "\n");

    BOOST_TEST_MESSAGE("Maximum results:");
    //maximum size
    auto read_after_write_max_size = std::max_element(read_after_write_times.cbegin(), read_after_write_times.cend(),
                                           [](const auto &a, const auto &b) { return std::get<1>(a) < std::get<1>(b); });
    BOOST_TEST_MESSAGE("Maximum size is " + std::to_string(std::get<1>(*read_after_write_max_size)) +
                       " from Block reference \"" + boost::algorithm::hex(
            std::string{std::get<0>(*read_after_write_max_size).cbegin(), std::get<0>(*read_after_write_max_size).cend()}) + "\" with a size of " +
                       std::to_string(std::get<0>(*read_after_write_max_size).size()) + " with an read after write time of " +
                       std::to_string(std::get<2>(*read_after_write_max_size)) + " ms");
    //maximum block ref size
    auto read_after_write_max_block_ref_size = std::max_element(read_after_write_times.cbegin(), read_after_write_times.cend(),
                                                     [](const auto &a, const auto &b) {
                                                         return std::get<0>(a).size() < std::get<0>(b).size();
                                                     });
    BOOST_TEST_MESSAGE("Maximum block reference size is " + std::to_string(std::get<0>(*read_after_write_max_block_ref_size).size()) +
                       " from Block reference \"" + boost::algorithm::hex(
            std::string{std::get<0>(*read_after_write_max_block_ref_size).cbegin(), std::get<0>(*read_after_write_max_block_ref_size).cend()}) +
                       "\" with a total block size of " + std::to_string(std::get<1>(*read_after_write_max_block_ref_size)) +
                       " with an read after write time of " +
                       std::to_string(std::get<2>(*read_after_write_max_block_ref_size)) + " ms");
    //maximum time taken
    auto read_after_write_max_time_taken = std::max_element(read_after_write_times.cbegin(), read_after_write_times.cend(), [](const auto &a, const auto &b) {
        return std::get<2>(a) < std::get<2>(b);
    });
    BOOST_TEST_MESSAGE("Maximum read after write time is " + std::to_string(std::get<2>(*read_after_write_max_time_taken)) +
                       " ms from Block reference \"" + boost::algorithm::hex(
            std::string{std::get<0>(*read_after_write_max_time_taken).cbegin(), std::get<0>(*read_after_write_max_time_taken).cend()}) +
                       "\" with a block reference size of " +
                       std::to_string(std::get<0>(*read_after_write_max_time_taken).size()) + " with a total block size of " +
                       std::to_string(std::get<1>(*read_after_write_max_time_taken)) + "\n");

    long double read_after_write_avg_size = 0;
    long double read_after_write_avg_block_ref_size = 0;
    long double read_after_write_avg_time = 0;
    for(const auto &i:read_after_write_times){
        read_after_write_avg_size+=std::get<1>(i);
        read_after_write_avg_size+=std::get<0>(i).size();
        read_after_write_avg_time+=std::get<2>(i);
    }
    read_after_write_avg_size/=read_after_write_times.size();
    read_after_write_avg_block_ref_size/=read_after_write_times.size();
    read_after_write_avg_time/=read_after_write_times.size();

    long double read_after_write_integration_speed_MB = (read_after_write_avg_size/std::pow(2,20))/(read_after_write_avg_time/1000);

    BOOST_TEST_MESSAGE("Average writing results:");
    BOOST_TEST_MESSAGE("Average read after write time is " + std::to_string(read_after_write_avg_time) +
                       " ms with an average block reference size of " +
                       std::to_string(read_after_write_avg_block_ref_size) + " with an average total block size of " +
                       std::to_string(read_after_write_avg_size) + ". This results in an average read after write speed of "+std::to_string(read_after_write_integration_speed_MB)+" MB per second\n");

}