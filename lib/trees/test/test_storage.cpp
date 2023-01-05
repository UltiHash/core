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
#include <cstdio>

std::vector<unsigned char> binary_generator(std::size_t max_len) {
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<std::mt19937::result_type> dist(0, max_len);

    std::size_t len = dist(rng);

    std::random_device dev3;
    std::mt19937 rng3(dev3());
    std::uniform_int_distribution<std::mt19937::result_type> dist3(0, UINT8_MAX);

    std::vector<unsigned char> out_return;
    out_return.resize(len);
    std::for_each(out_return.begin(),out_return.end(),[&dist3,&rng3](auto &a){
        a = dist3(rng3);
    });

    return out_return;
}

// ------------- Tests Follow --------------
/*
 * test setting up a tree storage at a given location
 */
BOOST_AUTO_TEST_CASE(constructor_test)
{
    //tests for any linux machine
    uh::trees::tree_storage t1(std::filesystem::path("/home") / std::string(getenv("USER")));//A test folder reserved for tree storage
    //for strong laptops with SSD extension (configure test db server to run this??)
    //uh::trees::tree_storage t1("/mnt/md0");//A test folder reserved for tree storage for performance tests
}

BOOST_AUTO_TEST_CASE(write_read_test)
{
    //tests for any linux machine
    uh::trees::tree_storage t1(std::filesystem::path("/home") / std::string(getenv("USER")));//A test folder reserved for tree storage
    //for strong laptops with SSD extension (configure test db server to run this??)
    //uh::trees::tree_storage t1("/mnt/md0");//A test folder reserved for tree storage for performance tests

    struct timeval time{};
    for (unsigned char mode = 0; mode < 2; mode++) {
        std::size_t total_size{};
        //list of write times with local_block_ref, integrated block size and milliseconds
        std::vector<std::tuple<std::vector<unsigned char>, std::size_t, long double>> write_times, read_after_write_times, linear_read_times, randam_access_read_times;
        //retrieved block size, local_block_ref size and time taken
        mode ? BOOST_TEST_MESSAGE("---Entering short block latency measurement write read mode:---\n") :
        BOOST_TEST_MESSAGE("---Entering normal write read mode:---\n");
        //total size is the total write size that the database tests
        while (total_size < (mode ? LATENCY_TEST_SIZE : PERFORMANCE_TEST_SIZE)) {
            //(std::size_t) std::pow(2, 35))
            std::vector<unsigned char> test_bin = binary_generator(mode ? 32 : STORE_MAX);
            //write test
            gettimeofday(&time, nullptr);
            long double millis = ((long double) time.tv_sec * 1000) + ((long double) time.tv_usec / 1000);
            std::vector<unsigned char> local_block_ref = t1.write(test_bin);
            gettimeofday(&time, nullptr);
            long double write_time =
                    (((long double) time.tv_sec * 1000) + ((long double) time.tv_usec / 1000)) - millis;
            BOOST_ASSERT_MSG(!local_block_ref.empty(), std::string(
                    "Database writing failed at block size " + std::to_string(test_bin.size()) + " at total size " +
                    std::to_string(total_size) + " . No reference retrieved!").c_str());
            if (local_block_ref.empty())continue;
            write_times.emplace_back(local_block_ref, test_bin.size(), write_time);
            //read after write test
            gettimeofday(&time, nullptr);
            millis = ((long double) time.tv_sec * 1000) + ((long double) time.tv_usec / 1000);
            auto all_result = t1.read(local_block_ref);
            std::vector<unsigned char> read_result = std::get<1>(all_result);
            gettimeofday(&time, nullptr);
            long double read_after_write_time =
                    (((long double) time.tv_sec * 1000) + ((long double) time.tv_usec / 1000)) - millis;
            //check correctness of stored string
            bool cmp = std::equal(test_bin.cbegin(), test_bin.cend(), read_result.cbegin(), read_result.cend());
            if(!cmp)all_result = t1.read(local_block_ref);
            BOOST_ASSERT_MSG(cmp, std::string("The write read result from block \"" + boost::algorithm::hex(
                    std::string(local_block_ref.cbegin(), local_block_ref.cend())) + "\" failed.").c_str());
            read_after_write_times.emplace_back(local_block_ref, read_result.size(), read_after_write_time);

            total_size += test_bin.size();
        }

        BOOST_ASSERT_MSG(!write_times.empty(), "No write times were tested!");
        //test sequential read from beginning on
        for (const auto &i: write_times) {
            gettimeofday(&time, nullptr);
            long double millis = ((long double) time.tv_sec * 1000) + ((long double) time.tv_usec / 1000);
            auto all_results = t1.read(std::get<0>(i));
            std::vector<unsigned char> read_result = std::get<1>(all_results);
            gettimeofday(&time, nullptr);
            long double read_sequential =
                    (((long double) time.tv_sec * 1000) + ((long double) time.tv_usec / 1000)) - millis;
            BOOST_ASSERT_MSG(!read_result.empty(), std::string(
                    "Database sequential reading failed at block reference " +
                    boost::algorithm::hex(std::string{std::get<0>(i).cbegin(), std::get<0>(i).cend()}) +
                    " . No block retrieved!").c_str());
            linear_read_times.emplace_back(std::get<0>(i), read_result.size(), read_sequential);
        }

        //test random access times for 256GB
        total_size = 0;
        std::random_device dev;
        std::mt19937 rng(dev());
        std::uniform_int_distribution<std::mt19937::result_type> dist(0, write_times.size() - 1);
        while (total_size < (mode ? LATENCY_TEST_SIZE : PERFORMANCE_TEST_SIZE)) {
            std::size_t access_point = dist(rng);
            gettimeofday(&time, nullptr);
            long double millis = ((long double) time.tv_sec * 1000) + ((long double) time.tv_usec / 1000);
            auto all_results = t1.read(std::get<0>(write_times[access_point]));
            std::vector<unsigned char> read_result = std::get<1>(all_results);
            gettimeofday(&time, nullptr);
            long double read_sequential =
                    (((long double) time.tv_sec * 1000) + ((long double) time.tv_usec / 1000)) - millis;
            BOOST_ASSERT_MSG(!read_result.empty(), std::string(
                    "Database random access reading failed at block reference " +
                    boost::algorithm::hex(std::string{std::get<0>(write_times[access_point]).cbegin(),
                                                      std::get<0>(write_times[access_point]).cend()}) +
                    " . No block retrieved!").c_str());
            randam_access_read_times.emplace_back(std::get<0>(write_times[access_point]), read_result.size(),
                                                  read_sequential);
            total_size += read_result.size();
        }

        //write times min, max, average on size/block ref size/time taken
        BOOST_TEST_MESSAGE("\nTest results for writing:\n");
        BOOST_TEST_MESSAGE("Minimum results:");
        //minimum size
        auto write_min_size = std::min_element(write_times.cbegin(), write_times.cend(),
                                               [](const auto &a, const auto &b) {
                                                   return std::get<1>(a) < std::get<1>(b);
                                               });
        BOOST_TEST_MESSAGE("Minimum size is " + std::to_string(std::get<1>(*write_min_size)) +
                           " from Block reference \"" + boost::algorithm::hex(
                std::string{std::get<0>(*write_min_size).cbegin(), std::get<0>(*write_min_size).cend()}) +
                           "\" with a block reference size of " +
                           std::to_string(std::get<0>(*write_min_size).size()) + " with an integration time of " +
                           std::to_string(std::get<2>(*write_min_size)) + " ms");
        //minimum block ref size
        auto write_min_block_ref_size = std::min_element(write_times.cbegin(), write_times.cend(),
                                                         [](const auto &a, const auto &b) {
                                                             return std::get<0>(a).size() < std::get<0>(b).size();
                                                         });
        BOOST_TEST_MESSAGE(
                "Minimum block reference size is " + std::to_string(std::get<0>(*write_min_block_ref_size).size()) +
                " from Block reference \"" + boost::algorithm::hex(
                        std::string{std::get<0>(*write_min_block_ref_size).cbegin(),
                                    std::get<0>(*write_min_block_ref_size).cend()}) +
                "\" with a total block size of " + std::to_string(std::get<1>(*write_min_block_ref_size)) +
                " with an integration time of " +
                std::to_string(std::get<2>(*write_min_block_ref_size)) + " ms");
        //minimum time taken
        auto write_min_time_taken = std::min_element(write_times.cbegin(), write_times.cend(),
                                                     [](const auto &a, const auto &b) {
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
                                               [](const auto &a, const auto &b) {
                                                   return std::get<1>(a) < std::get<1>(b);
                                               });
        BOOST_TEST_MESSAGE("Maximum size is " + std::to_string(std::get<1>(*write_max_size)) +
                           " from Block reference \"" + boost::algorithm::hex(
                std::string{std::get<0>(*write_max_size).cbegin(), std::get<0>(*write_max_size).cend()}) +
                           "\" with a block reference size of " +
                           std::to_string(std::get<0>(*write_max_size).size()) + " with an integration time of " +
                           std::to_string(std::get<2>(*write_max_size)) + " ms");
        //maximum block ref size
        auto write_max_block_ref_size = std::max_element(write_times.cbegin(), write_times.cend(),
                                                         [](const auto &a, const auto &b) {
                                                             return std::get<0>(a).size() < std::get<0>(b).size();
                                                         });
        BOOST_TEST_MESSAGE(
                "Maximum block reference size is " + std::to_string(std::get<0>(*write_max_block_ref_size).size()) +
                " from Block reference \"" + boost::algorithm::hex(
                        std::string{std::get<0>(*write_max_block_ref_size).cbegin(),
                                    std::get<0>(*write_max_block_ref_size).cend()}) +
                "\" with a total block size of " + std::to_string(std::get<1>(*write_max_block_ref_size)) +
                " with an integration time of " +
                std::to_string(std::get<2>(*write_max_block_ref_size)) + " ms");
        //maximum time taken
        auto write_max_time_taken = std::max_element(write_times.cbegin(), write_times.cend(),
                                                     [](const auto &a, const auto &b) {
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
        for (const auto &i: write_times) {
            write_avg_size += std::get<1>(i);
            write_avg_block_ref_size += std::get<0>(i).size();
            write_avg_time += std::get<2>(i);
        }
        long double write_total_time = write_avg_time;
        write_avg_size /= write_times.size();
        write_avg_block_ref_size /= write_times.size();
        write_avg_time /= write_times.size();

        long double write_integration_speed_MB = (write_avg_size / std::pow(2, 20)) / (write_avg_time / 1000);

        BOOST_TEST_MESSAGE("Average writing results:");
        BOOST_TEST_MESSAGE("Average integration time is " + std::to_string(write_avg_time) +
                           " ms with an average block reference size of " +
                           std::to_string(write_avg_block_ref_size) + " with an average total block size of " +
                           std::to_string(write_avg_size) + ". This results in an average integration speed of " +
                           std::to_string(write_integration_speed_MB) + " MB per second\n");
        BOOST_TEST_MESSAGE("The total time taken to write " + std::to_string(total_size) + " bytes was " +
                           std::to_string(write_total_time) + " ms");
        //show read after write results
        BOOST_TEST_MESSAGE("\nTest results for read after write:\n");
        BOOST_TEST_MESSAGE("Minimum results:");
        //minimum size
        auto read_after_write_min_size = std::min_element(read_after_write_times.cbegin(),
                                                          read_after_write_times.cend(),
                                                          [](const auto &a, const auto &b) {
                                                              return std::get<1>(a) < std::get<1>(b);
                                                          });
        BOOST_TEST_MESSAGE("Minimum size is " + std::to_string(std::get<1>(*read_after_write_min_size)) +
                           " from Block reference \"" + boost::algorithm::hex(
                std::string{std::get<0>(*read_after_write_min_size).cbegin(),
                            std::get<0>(*read_after_write_min_size).cend()}) + "\" with a block reference size of " +
                           std::to_string(std::get<0>(*read_after_write_min_size).size()) +
                           " with a read after write time of " +
                           std::to_string(std::get<2>(*read_after_write_min_size)) + " ms");
        //minimum block ref size
        auto read_after_write_min_block_ref_size = std::min_element(read_after_write_times.cbegin(),
                                                                    read_after_write_times.cend(),
                                                                    [](const auto &a, const auto &b) {
                                                                        return std::get<0>(a).size() <
                                                                               std::get<0>(b).size();
                                                                    });
        BOOST_TEST_MESSAGE("Minimum block reference size is " +
                           std::to_string(std::get<0>(*read_after_write_min_block_ref_size).size()) +
                           " from Block reference \"" + boost::algorithm::hex(
                std::string{std::get<0>(*read_after_write_min_block_ref_size).cbegin(),
                            std::get<0>(*read_after_write_min_block_ref_size).cend()}) +
                           "\" with a total block size of " +
                           std::to_string(std::get<1>(*read_after_write_min_block_ref_size)) +
                           " with a read after write time of " +
                           std::to_string(std::get<2>(*read_after_write_min_block_ref_size)) + " ms");
        //minimum time taken
        auto read_after_write_min_time_taken = std::min_element(read_after_write_times.cbegin(),
                                                                read_after_write_times.cend(),
                                                                [](const auto &a, const auto &b) {
                                                                    return std::get<2>(a) < std::get<2>(b);
                                                                });
        BOOST_TEST_MESSAGE(
                "Minimum read after write time is " + std::to_string(std::get<2>(*read_after_write_min_time_taken)) +
                " ms from Block reference \"" + boost::algorithm::hex(
                        std::string{std::get<0>(*read_after_write_min_time_taken).cbegin(),
                                    std::get<0>(*read_after_write_min_time_taken).cend()}) +
                "\" with a block reference size of " +
                std::to_string(std::get<0>(*read_after_write_min_time_taken).size()) + " with a total block size of " +
                std::to_string(std::get<1>(*read_after_write_min_time_taken)) + "\n");

        BOOST_TEST_MESSAGE("Maximum results:");
        //maximum size
        auto read_after_write_max_size = std::max_element(read_after_write_times.cbegin(),
                                                          read_after_write_times.cend(),
                                                          [](const auto &a, const auto &b) {
                                                              return std::get<1>(a) < std::get<1>(b);
                                                          });
        BOOST_TEST_MESSAGE("Maximum size is " + std::to_string(std::get<1>(*read_after_write_max_size)) +
                           " from Block reference \"" + boost::algorithm::hex(
                std::string{std::get<0>(*read_after_write_max_size).cbegin(),
                            std::get<0>(*read_after_write_max_size).cend()}) + "\" with a block reference size of " +
                           std::to_string(std::get<0>(*read_after_write_max_size).size()) +
                           " with a read after write time of " +
                           std::to_string(std::get<2>(*read_after_write_max_size)) + " ms");
        //maximum block ref size
        auto read_after_write_max_block_ref_size = std::max_element(read_after_write_times.cbegin(),
                                                                    read_after_write_times.cend(),
                                                                    [](const auto &a, const auto &b) {
                                                                        return std::get<0>(a).size() <
                                                                               std::get<0>(b).size();
                                                                    });
        BOOST_TEST_MESSAGE("Maximum block reference size is " +
                           std::to_string(std::get<0>(*read_after_write_max_block_ref_size).size()) +
                           " from Block reference \"" + boost::algorithm::hex(
                std::string{std::get<0>(*read_after_write_max_block_ref_size).cbegin(),
                            std::get<0>(*read_after_write_max_block_ref_size).cend()}) +
                           "\" with a total block size of " +
                           std::to_string(std::get<1>(*read_after_write_max_block_ref_size)) +
                           " with a read after write time of " +
                           std::to_string(std::get<2>(*read_after_write_max_block_ref_size)) + " ms");
        //maximum time taken
        auto read_after_write_max_time_taken = std::max_element(read_after_write_times.cbegin(),
                                                                read_after_write_times.cend(),
                                                                [](const auto &a, const auto &b) {
                                                                    return std::get<2>(a) < std::get<2>(b);
                                                                });
        BOOST_TEST_MESSAGE(
                "Maximum read after write time is " + std::to_string(std::get<2>(*read_after_write_max_time_taken)) +
                " ms from Block reference \"" + boost::algorithm::hex(
                        std::string{std::get<0>(*read_after_write_max_time_taken).cbegin(),
                                    std::get<0>(*read_after_write_max_time_taken).cend()}) +
                "\" with a block reference size of " +
                std::to_string(std::get<0>(*read_after_write_max_time_taken).size()) + " with a total block size of " +
                std::to_string(std::get<1>(*read_after_write_max_time_taken)) + "\n");

        long double read_after_write_avg_size = 0;
        long double read_after_write_avg_block_ref_size = 0;
        long double read_after_write_avg_time = 0;
        for (const auto &i: read_after_write_times) {
            read_after_write_avg_size += std::get<1>(i);
            read_after_write_avg_block_ref_size += std::get<0>(i).size();
            read_after_write_avg_time += std::get<2>(i);
        }
        long double read_after_write_total_time = read_after_write_avg_time;
        read_after_write_avg_size /= read_after_write_times.size();
        read_after_write_avg_block_ref_size /= read_after_write_times.size();
        read_after_write_avg_time /= read_after_write_times.size();

        long double read_after_write_integration_speed_MB =
                (read_after_write_avg_size / std::pow(2, 20)) / (read_after_write_avg_time / 1000);

        BOOST_TEST_MESSAGE("Average read after write results:");
        BOOST_TEST_MESSAGE("Average read after write time is " + std::to_string(read_after_write_avg_time) +
                           " ms with an average block reference size of " +
                           std::to_string(read_after_write_avg_block_ref_size) +
                           " with an average total block size of " +
                           std::to_string(read_after_write_avg_size) +
                           ". This results in an average read after write speed of " +
                           std::to_string(read_after_write_integration_speed_MB) + " MB per second\n");
        BOOST_TEST_MESSAGE("The total time taken to read after write " + std::to_string(total_size) + " bytes was " +
                           std::to_string(read_after_write_total_time) + " ms");
        //show linear read results
        BOOST_TEST_MESSAGE("\nTest results for linear read:\n");
        BOOST_TEST_MESSAGE("Minimum results:");
        //minimum size
        auto linear_read_min_size = std::min_element(linear_read_times.cbegin(), linear_read_times.cend(),
                                                     [](const auto &a, const auto &b) {
                                                         return std::get<1>(a) < std::get<1>(b);
                                                     });
        BOOST_TEST_MESSAGE("Minimum size is " + std::to_string(std::get<1>(*linear_read_min_size)) +
                           " from Block reference \"" + boost::algorithm::hex(
                std::string{std::get<0>(*linear_read_min_size).cbegin(), std::get<0>(*linear_read_min_size).cend()}) +
                           "\" with a block reference size of " +
                           std::to_string(std::get<0>(*linear_read_min_size).size()) + " with a linear read time of " +
                           std::to_string(std::get<2>(*linear_read_min_size)) + " ms");
        //minimum block ref size
        auto linear_read_min_block_ref_size = std::min_element(linear_read_times.cbegin(), linear_read_times.cend(),
                                                               [](const auto &a, const auto &b) {
                                                                   return std::get<0>(a).size() < std::get<0>(b).size();
                                                               });
        BOOST_TEST_MESSAGE("Minimum block reference size is " +
                           std::to_string(std::get<0>(*linear_read_min_block_ref_size).size()) +
                           " from Block reference \"" + boost::algorithm::hex(
                std::string{std::get<0>(*linear_read_min_block_ref_size).cbegin(),
                            std::get<0>(*linear_read_min_block_ref_size).cend()}) +
                           "\" with a total block size of " +
                           std::to_string(std::get<1>(*linear_read_min_block_ref_size)) +
                           " with a linear read time of " +
                           std::to_string(std::get<2>(*linear_read_min_block_ref_size)) + " ms");
        //minimum time taken
        auto linear_read_min_time_taken = std::min_element(linear_read_times.cbegin(), linear_read_times.cend(),
                                                           [](const auto &a, const auto &b) {
                                                               return std::get<2>(a) < std::get<2>(b);
                                                           });
        BOOST_TEST_MESSAGE("Minimum linear read time is " + std::to_string(std::get<2>(*linear_read_min_time_taken)) +
                           " ms from Block reference \"" + boost::algorithm::hex(
                std::string{std::get<0>(*linear_read_min_time_taken).cbegin(),
                            std::get<0>(*linear_read_min_time_taken).cend()}) +
                           "\" with a block reference size of " +
                           std::to_string(std::get<0>(*linear_read_min_time_taken).size()) +
                           " with a total block size of " +
                           std::to_string(std::get<1>(*linear_read_min_time_taken)) + "\n");

        BOOST_TEST_MESSAGE("Maximum results:");
        //maximum size
        auto linear_read_max_size = std::max_element(linear_read_times.cbegin(), linear_read_times.cend(),
                                                     [](const auto &a, const auto &b) {
                                                         return std::get<1>(a) < std::get<1>(b);
                                                     });
        BOOST_TEST_MESSAGE("Maximum size is " + std::to_string(std::get<1>(*linear_read_max_size)) +
                           " from Block reference \"" + boost::algorithm::hex(
                std::string{std::get<0>(*linear_read_max_size).cbegin(), std::get<0>(*linear_read_max_size).cend()}) +
                           "\" with a block reference size of " +
                           std::to_string(std::get<0>(*linear_read_max_size).size()) + " with a linear read time of " +
                           std::to_string(std::get<2>(*linear_read_max_size)) + " ms");
        //maximum block ref size
        auto linear_read_max_block_ref_size = std::max_element(linear_read_times.cbegin(), linear_read_times.cend(),
                                                               [](const auto &a, const auto &b) {
                                                                   return std::get<0>(a).size() < std::get<0>(b).size();
                                                               });
        BOOST_TEST_MESSAGE("Maximum block reference size is " +
                           std::to_string(std::get<0>(*linear_read_max_block_ref_size).size()) +
                           " from Block reference \"" + boost::algorithm::hex(
                std::string{std::get<0>(*linear_read_max_block_ref_size).cbegin(),
                            std::get<0>(*linear_read_max_block_ref_size).cend()}) +
                           "\" with a total block size of " +
                           std::to_string(std::get<1>(*linear_read_max_block_ref_size)) +
                           " with a linear read time of " +
                           std::to_string(std::get<2>(*linear_read_max_block_ref_size)) + " ms");
        //maximum time taken
        auto linear_read_max_time_taken = std::max_element(linear_read_times.cbegin(), linear_read_times.cend(),
                                                           [](const auto &a, const auto &b) {
                                                               return std::get<2>(a) < std::get<2>(b);
                                                           });
        BOOST_TEST_MESSAGE("Maximum linear read time is " + std::to_string(std::get<2>(*linear_read_max_time_taken)) +
                           " ms from Block reference \"" + boost::algorithm::hex(
                std::string{std::get<0>(*linear_read_max_time_taken).cbegin(),
                            std::get<0>(*linear_read_max_time_taken).cend()}) +
                           "\" with a block reference size of " +
                           std::to_string(std::get<0>(*linear_read_max_time_taken).size()) +
                           " with a total block size of " +
                           std::to_string(std::get<1>(*linear_read_max_time_taken)) + "\n");

        long double linear_read_avg_size = 0;
        long double linear_read_avg_block_ref_size = 0;
        long double linear_read_avg_time = 0;
        for (const auto &i: linear_read_times) {
            linear_read_avg_size += std::get<1>(i);
            linear_read_avg_block_ref_size += std::get<0>(i).size();
            linear_read_avg_time += std::get<2>(i);
        }
        long double linear_read_total_time = linear_read_avg_time;
        linear_read_avg_size /= linear_read_times.size();
        linear_read_avg_block_ref_size /= linear_read_times.size();
        linear_read_avg_time /= linear_read_times.size();

        long double linear_read_integration_speed_MB =
                (linear_read_avg_size / std::pow(2, 20)) / (linear_read_avg_time / 1000);

        BOOST_TEST_MESSAGE("Average linear read results:");
        BOOST_TEST_MESSAGE("Average linear read time is " + std::to_string(linear_read_avg_time) +
                           " ms with an average block reference size of " +
                           std::to_string(linear_read_avg_block_ref_size) + " with an average total block size of " +
                           std::to_string(linear_read_avg_size) + ". This results in an average linear read speed of " +
                           std::to_string(linear_read_integration_speed_MB) + " MB per second\n");
        BOOST_TEST_MESSAGE("The total time taken to linear read " + std::to_string(total_size) + " bytes was " +
                           std::to_string(linear_read_total_time) + " ms");
        //show random access read results
        BOOST_TEST_MESSAGE("\nTest results for random access read:\n");
        BOOST_TEST_MESSAGE("Minimum results:");
        //minimum size
        auto randam_access_read_min_size = std::min_element(randam_access_read_times.cbegin(),
                                                            randam_access_read_times.cend(),
                                                            [](const auto &a, const auto &b) {
                                                                return std::get<1>(a) < std::get<1>(b);
                                                            });
        BOOST_TEST_MESSAGE("Minimum size is " + std::to_string(std::get<1>(*randam_access_read_min_size)) +
                           " from Block reference \"" + boost::algorithm::hex(
                std::string{std::get<0>(*randam_access_read_min_size).cbegin(),
                            std::get<0>(*randam_access_read_min_size).cend()}) + "\" with a block reference size of " +
                           std::to_string(std::get<0>(*randam_access_read_min_size).size()) +
                           " with a random access read time of " +
                           std::to_string(std::get<2>(*randam_access_read_min_size)) + " ms");
        //minimum block ref size
        auto randam_access_read_min_block_ref_size = std::min_element(randam_access_read_times.cbegin(),
                                                                      randam_access_read_times.cend(),
                                                                      [](const auto &a, const auto &b) {
                                                                          return std::get<0>(a).size() <
                                                                                 std::get<0>(b).size();
                                                                      });
        BOOST_TEST_MESSAGE("Minimum block reference size is " +
                           std::to_string(std::get<0>(*randam_access_read_min_block_ref_size).size()) +
                           " from Block reference \"" + boost::algorithm::hex(
                std::string{std::get<0>(*randam_access_read_min_block_ref_size).cbegin(),
                            std::get<0>(*randam_access_read_min_block_ref_size).cend()}) +
                           "\" with a total block size of " +
                           std::to_string(std::get<1>(*randam_access_read_min_block_ref_size)) +
                           " with a random access read time of " +
                           std::to_string(std::get<2>(*randam_access_read_min_block_ref_size)) + " ms");
        //minimum time taken
        auto randam_access_read_min_time_taken = std::min_element(randam_access_read_times.cbegin(),
                                                                  randam_access_read_times.cend(),
                                                                  [](const auto &a, const auto &b) {
                                                                      return std::get<2>(a) < std::get<2>(b);
                                                                  });
        BOOST_TEST_MESSAGE("Minimum random access read time is " +
                           std::to_string(std::get<2>(*randam_access_read_min_time_taken)) +
                           " ms from Block reference \"" + boost::algorithm::hex(
                std::string{std::get<0>(*randam_access_read_min_time_taken).cbegin(),
                            std::get<0>(*randam_access_read_min_time_taken).cend()}) +
                           "\" with a block reference size of " +
                           std::to_string(std::get<0>(*randam_access_read_min_time_taken).size()) +
                           " with a total block size of " +
                           std::to_string(std::get<1>(*randam_access_read_min_time_taken)) + "\n");

        BOOST_TEST_MESSAGE("Maximum results:");
        //maximum size
        auto randam_access_read_max_size = std::max_element(randam_access_read_times.cbegin(),
                                                            randam_access_read_times.cend(),
                                                            [](const auto &a, const auto &b) {
                                                                return std::get<1>(a) < std::get<1>(b);
                                                            });
        BOOST_TEST_MESSAGE("Maximum size is " + std::to_string(std::get<1>(*randam_access_read_max_size)) +
                           " from Block reference \"" + boost::algorithm::hex(
                std::string{std::get<0>(*randam_access_read_max_size).cbegin(),
                            std::get<0>(*randam_access_read_max_size).cend()}) + "\" with a block reference size of " +
                           std::to_string(std::get<0>(*randam_access_read_max_size).size()) +
                           " with a random access read time of " +
                           std::to_string(std::get<2>(*randam_access_read_max_size)) + " ms");
        //maximum block ref size
        auto randam_access_read_max_block_ref_size = std::max_element(randam_access_read_times.cbegin(),
                                                                      randam_access_read_times.cend(),
                                                                      [](const auto &a, const auto &b) {
                                                                          return std::get<0>(a).size() <
                                                                                 std::get<0>(b).size();
                                                                      });
        BOOST_TEST_MESSAGE("Maximum block reference size is " +
                           std::to_string(std::get<0>(*randam_access_read_max_block_ref_size).size()) +
                           " from Block reference \"" + boost::algorithm::hex(
                std::string{std::get<0>(*randam_access_read_max_block_ref_size).cbegin(),
                            std::get<0>(*randam_access_read_max_block_ref_size).cend()}) +
                           "\" with a total block size of " +
                           std::to_string(std::get<1>(*randam_access_read_max_block_ref_size)) +
                           " with a random access read time of " +
                           std::to_string(std::get<2>(*randam_access_read_max_block_ref_size)) + " ms");
        //maximum time taken
        auto randam_access_read_max_time_taken = std::max_element(randam_access_read_times.cbegin(),
                                                                  randam_access_read_times.cend(),
                                                                  [](const auto &a, const auto &b) {
                                                                      return std::get<2>(a) < std::get<2>(b);
                                                                  });
        BOOST_TEST_MESSAGE("Maximum random access read time is " +
                           std::to_string(std::get<2>(*randam_access_read_max_time_taken)) +
                           " ms from Block reference \"" + boost::algorithm::hex(
                std::string{std::get<0>(*randam_access_read_max_time_taken).cbegin(),
                            std::get<0>(*randam_access_read_max_time_taken).cend()}) +
                           "\" with a block reference size of " +
                           std::to_string(std::get<0>(*randam_access_read_max_time_taken).size()) +
                           " with a total block size of " +
                           std::to_string(std::get<1>(*randam_access_read_max_time_taken)) + "\n");

        long double randam_access_read_avg_size = 0;
        long double randam_access_read_avg_block_ref_size = 0;
        long double randam_access_read_avg_time = 0;
        for (const auto &i: randam_access_read_times) {
            randam_access_read_avg_size += std::get<1>(i);
            randam_access_read_avg_block_ref_size += std::get<0>(i).size();
            randam_access_read_avg_time += std::get<2>(i);
        }
        long double random_access_read_total_time = randam_access_read_avg_time;
        randam_access_read_avg_size /= randam_access_read_times.size();
        randam_access_read_avg_block_ref_size /= randam_access_read_times.size();
        randam_access_read_avg_time /= randam_access_read_times.size();

        long double randam_access_read_integration_speed_MB =
                (randam_access_read_avg_size / std::pow(2, 20)) / (randam_access_read_avg_time / 1000);

        BOOST_TEST_MESSAGE("Average random access read results:");
        BOOST_TEST_MESSAGE("Average random access read time is " + std::to_string(randam_access_read_avg_time) +
                           " ms with an average block reference size of " +
                           std::to_string(randam_access_read_avg_block_ref_size) +
                           " with an average total block size of " +
                           std::to_string(randam_access_read_avg_size) +
                           ". This results in an average random access read speed of " +
                           std::to_string(randam_access_read_integration_speed_MB) + " MB per second\n");
        BOOST_TEST_MESSAGE("The total time taken to random access read " + std::to_string(total_size) + " bytes was " +
                           std::to_string(random_access_read_total_time) + " ms");
    }
}
//TODO: make thread safe
BOOST_AUTO_TEST_CASE(index_read_test)
{
    //get entire index, read block with local_block_reference and finally calculate the hash if it matches
    struct timeval time{};
    gettimeofday(&time, nullptr);
    long double millis = ((long double) time.tv_sec * 1000) + ((long double) time.tv_usec / 1000);
    //for any machine
    uh::trees::tree_storage t1(std::filesystem::path("/home") / std::string(getenv("USER")),
                               1);//A test folder reserved for tree storage
    //for strong laptops with SSD extension (configure test db server to run this??)
    //uh::trees::tree_storage t1("/mnt/md0");//A test folder reserved for tree storage for performance tests
    gettimeofday(&time, nullptr);
    long double constructor_time = (((long double) time.tv_sec * 1000) + ((long double) time.tv_usec / 1000)) - millis;

    gettimeofday(&time, nullptr);
    millis = ((long double) time.tv_sec * 1000) + ((long double) time.tv_usec / 1000);
    auto index_list = t1.index(1);
    gettimeofday(&time, nullptr);
    long double index_time = (((long double) time.tv_sec * 1000) + ((long double) time.tv_usec / 1000)) - millis;

    BOOST_TEST_MESSAGE("The constructor of tree storage took " + std::to_string(constructor_time) +
                       " ms in a full state. The index time of that tree took " + std::to_string(index_time) +
                       " ms. The total time to bring the database back to life was " +
                       std::to_string(constructor_time + index_time) + " ms.");

    for (const auto &el: index_list) {
        auto all_results = t1.read(std::get<1>(el));
        std::vector<unsigned char> read_result = std::get<1>(all_results);
        unsigned char hash_buf[SHA512_DIGEST_LENGTH];//HASH GENERATION
        SHA512(read_result.data(), read_result.size(), hash_buf);
        std::string old_ref = boost::algorithm::hex(
                std::string().assign(std::get<1>(el).cbegin(), std::get<1>(el).cend()));
        bool test_ok = std::equal(std::get<0>(el).cbegin(), std::get<0>(el).cend(), hash_buf, hash_buf + SHA512_DIGEST_LENGTH);
        BOOST_ASSERT_MSG(test_ok,std::string("The SHA512 of an indexed block \""+old_ref+"\" could not be verified!").c_str());
    }
}

BOOST_AUTO_TEST_CASE(get_info_set_time_test)
{
    struct timeval time{};
    //tests for any linux machine
    uh::trees::tree_storage t1(std::filesystem::path("/home") / std::string(getenv("USER")));//A test folder reserved for tree storage
    //for strong laptops with SSD extension (configure test db server to run this??)
    //uh::trees::tree_storage t1("/mnt/md0");//A test folder reserved for tree storage for performance tests
    auto index_list = t1.index(1);
    BOOST_ASSERT_MSG(!index_list.empty(),"Index list was empty!");
    auto first_el = index_list.begin();
    gettimeofday(&time, nullptr);
    long double millis = ((long double) time.tv_sec * 1000) + ((long double) time.tv_usec / 1000);
    auto block_info = t1.get_info(std::get<1>(*first_el));
    gettimeofday(&time, nullptr);
    long double write_time =
            (((long double) time.tv_sec * 1000) + ((long double) time.tv_usec / 1000)) - millis;
    BOOST_TEST_MESSAGE(
            "\nThe get_info test to seek the information of one block took " + std::to_string(write_time) + " ms.");
    BOOST_ASSERT_MSG(std::get<0>(block_info) < (unsigned long) std::chrono::nanoseconds(
            std::chrono::high_resolution_clock::now().time_since_epoch()).count(),
                     "Block was not older than current time");
    BOOST_ASSERT_MSG(std::get<1>(block_info) < std::get<2>(block_info),
                     "The total size must always be larger than the block size!");
    gettimeofday(&time, nullptr);
    millis = ((long double) time.tv_sec * 1000) + ((long double) time.tv_usec / 1000);
    bool test_ok = t1.set_block_time(std::get<1>(*first_el));
    gettimeofday(&time, nullptr);
    write_time = (((long double) time.tv_sec * 1000) + ((long double) time.tv_usec / 1000)) - millis;
    BOOST_ASSERT_MSG(test_ok, "Current time could not be set to first block of index!");
    BOOST_TEST_MESSAGE("\nSetting a new block time took " + std::to_string(write_time) + " ms.");
    auto block_info2 = t1.get_info(std::get<1>(*first_el));
    BOOST_ASSERT_MSG(std::get<0>(block_info) < std::get<0>(block_info2),
                     "Block time reset was not successful internally!");
    t1.delete_recursive();
}

BOOST_AUTO_TEST_CASE(delete_test)
{
    //tests for any linux machine
    uh::trees::tree_storage t1(
            std::filesystem::path("/home") / std::string(getenv("USER")));//A test folder reserved for tree storage
    //for strong laptops with SSD extension (configure test db server to run this??)
    //uh::trees::tree_storage t1("/mnt/md0");//A test folder reserved for tree storage for performance tests
    auto index_list = t1.index(1);
    std::vector<std::vector<unsigned char>> to_del;
    //from index take 2 blocks of the same chunk and copy them to RAM
    //delete one block over its reference and check if the block of the retured local reference is the same
    std::tuple<std::size_t, std::list<std::tuple<std::vector<unsigned char>, std::vector<unsigned char>>>> delete_list{};

    do {
        auto del_list = std::vector<std::vector<unsigned char>>{std::get<1>(*index_list.cbegin())};
        delete_list = t1.delete_blocks(del_list,1);
        index_list.pop_front();
    } while (std::get<1>(delete_list).empty());

    for (const auto &i: std::get<1>(delete_list)) {
        std::string old_ref = boost::algorithm::hex(
                std::string().assign(std::get<0>(i).cbegin(), std::get<0>(i).cend()));
        std::string new_ref = boost::algorithm::hex(
                std::string().assign(std::get<1>(i).cbegin(), std::get<1>(i).cend()));
        auto read_result = t1.read(std::get<1>(i));
        bool test_ok = !std::get<1>(read_result).empty();
        BOOST_ASSERT_MSG(test_ok, std::string("Block with old reference " + old_ref + " and new reference " + new_ref +
                                              " could not be read back after deletion!").c_str());
    }


}
