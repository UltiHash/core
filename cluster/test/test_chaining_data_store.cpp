//
// Created by masi on 7/24/23.
//

#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "chaining_data_store tests"
#endif

#include <boost/test/unit_test.hpp>
#include "common/common.h"
#include "data_node/free_spot_manager.h"
#include "directory_node/chaining_data_store.h"

// ------------- Tests Suites Follow --------------

namespace uh::cluster {

// ---------------------------------------------------------------------

struct config_fixture
{
    static uh::cluster::chaining_data_store_config make_chaining_data_store_config () {
        return {
                .directory = "root/dn",
                .free_spot_log = "root/dn/log",
                .min_file_size = 1024ul,
                .max_file_size = 8ul * 1024ul,
                .max_storage_size = 16 * 1024ul,
                .max_chunk_size = 16 * 1024ul,
        };
    }

    static void cleanup () {
        std::filesystem::remove_all("root");
    }
};


// ---------------------------------------------------------------------

void fill_random2(char* buf, size_t size) {
    for (int i = 0; i < size; ++i) {
        buf[i] = rand()&0xff;
    }
}

BOOST_FIXTURE_TEST_CASE (test_chaining_data_store, config_fixture)
{

    cleanup();

    char data1 [512];
    char data2 [1024];
    char data3 [164];
    char data4 [1520];
    char data5 [2572];
    char data6 [3021];
    char data7 [102];
    char data8 [5021];
    char data9 [2048];
    char data10 [3202];
    char data11 [2021];

    fill_random2 (data1, sizeof (data1));
    fill_random2 (data2, sizeof (data2));
    fill_random2 (data3, sizeof (data3));
    fill_random2 (data4, sizeof (data4));
    fill_random2 (data5, sizeof (data5));
    fill_random2 (data6, sizeof (data6));
    fill_random2 (data7, sizeof (data7));
    fill_random2 (data8, sizeof (data8));
    fill_random2 (data9, sizeof (data9));
    fill_random2 (data10, sizeof (data10));
    fill_random2 (data11, sizeof (data11));


    char buf [8*1024];
    char zero [8*1024];
    std::memset(zero, 0, sizeof(zero));

    uint64_t addr1, addr2, addr3, addr4, addr5, addr6, addr7, addr8, addr9, addr10, addr11;

    size_t expected_size = sizeof (size_t);
    {

        chaining_data_store ds (make_chaining_data_store_config());
        addr1 = ds.write(data1);
        expected_size += sizeof (data1) + sizeof (uint32_t);
        BOOST_CHECK(ds.get_used_space() == expected_size);
        addr2 = ds.write(data2);
        expected_size += sizeof (data2) + sizeof (uint32_t);
        BOOST_CHECK(ds.get_used_space() == expected_size);
        addr3 = ds.write(data3);
        expected_size += sizeof (data3) + sizeof (uint32_t);
        BOOST_CHECK(ds.get_used_space() == expected_size);
        addr4 = ds.write(data4);
        expected_size += sizeof (data4) + sizeof (uint32_t);
        addr5 = ds.write(data5);
        expected_size += sizeof (data5) + sizeof (uint32_t);
        BOOST_CHECK(ds.get_used_space() == expected_size);
        addr6 = ds.write(data6);
        expected_size += sizeof (data6) + sizeof (std::size_t) + 2 * sizeof (uint32_t) + 2 * sizeof (uint64_t); // new file
        BOOST_TEST(ds.get_used_space() == expected_size);
        addr7 = ds.write(data7);
        expected_size += sizeof (data7) + sizeof (uint32_t);
        addr8 = ds.write(data8);
        expected_size += sizeof (data8) + sizeof (uint32_t);
        addr9 = ds.write(data9);
        expected_size += sizeof (data9) + sizeof (uint32_t);
        BOOST_TEST(ds.get_used_space() == expected_size);

        BOOST_CHECK_THROW (ds.write(data10), std::bad_alloc);
        BOOST_TEST(ds.get_used_space() == expected_size);

        const auto d1 = ds.read(addr1);
        BOOST_TEST(d1.size == sizeof(data1));
        BOOST_CHECK(std::memcmp(d1.data.get(), data1, d1.size) == 0);

        const auto d2 = ds.read(addr2);
        BOOST_TEST(d2.size == sizeof(data2));
        BOOST_CHECK(std::memcmp(d2.data.get(), data2, d2.size) == 0);

        const auto d3 = ds.read(addr3);
        BOOST_TEST(d3.size == sizeof(data3));
        BOOST_CHECK(std::memcmp(d3.data.get(), data3, d3.size) == 0);

        const auto d4 = ds.read(addr4);
        BOOST_TEST(d4.size == sizeof(data4));
        BOOST_CHECK(std::memcmp(d4.data.get(), data4, d4.size) == 0);

        const auto d5 = ds.read(addr5);
        BOOST_TEST(d5.size == sizeof(data5));
        BOOST_CHECK(std::memcmp(d5.data.get(), data5, d5.size) == 0);

        const auto d6 = ds.read(addr6);
        BOOST_TEST(d6.size == sizeof(data6));
        BOOST_CHECK(std::memcmp(d6.data.get(), data6, d6.size) == 0);

        const auto d7 = ds.read(addr7);
        BOOST_TEST(d7.size == sizeof(data7));
        BOOST_CHECK(std::memcmp(d7.data.get(), data7, d7.size) == 0);

        const auto d8 = ds.read(addr8);
        BOOST_TEST(d8.size == sizeof(data8));
        BOOST_CHECK(std::memcmp(d8.data.get(), data8, d8.size) == 0);

        const auto d9 = ds.read(addr9);
        BOOST_TEST(d9.size == sizeof(data9));
        BOOST_CHECK(std::memcmp(d9.data.get(), data9, d9.size) == 0);

        BOOST_CHECK(ds.get_used_space() == expected_size);

        ds.remove(addr9);
        const auto dd9 = ds.read(addr9);
        expected_size -= sizeof(data9) + sizeof (uint32_t);
        BOOST_TEST(dd9.size == sizeof (data9));
        BOOST_CHECK(std::memcmp(buf, zero, dd9.size ) == 0);

        BOOST_CHECK_THROW (ds.write(data10), std::bad_alloc);
        BOOST_CHECK(ds.get_used_space() == expected_size);

        ds.remove(addr2);
        const auto dd2 = ds.read(addr2);
        expected_size -= sizeof(data2) + sizeof (uint32_t);
        BOOST_TEST(dd2.size == sizeof (data2));
        BOOST_CHECK(std::memcmp(buf, zero, dd2.size ) == 0);
        BOOST_CHECK(ds.get_used_space() == expected_size);

        const auto addr10 = ds.write(data10);
        expected_size += sizeof (data10) + 3 * sizeof (uint32_t) + sizeof (size_t) + 2*sizeof(uint64_t);
        BOOST_TEST (ds.get_used_space() == expected_size);

        BOOST_CHECK_THROW (ds.write(data11), std::bad_alloc);

        const auto dd1 = ds.read(addr1);
        BOOST_TEST(dd1.size == sizeof(data1));
        BOOST_CHECK(std::memcmp(dd1.data.get(), data1, dd1.size) == 0);

        const auto dd3 = ds.read(addr3);
        BOOST_TEST(dd3.size == sizeof(data3));
        BOOST_CHECK(std::memcmp(dd3.data.get(), data3, dd3.size) == 0);

        const auto dd4 = ds.read(addr4);
        BOOST_TEST(dd4.size == sizeof(data4));
        BOOST_CHECK(std::memcmp(dd4.data.get(), data4, dd4.size) == 0);

        const auto dd5 = ds.read(addr5);
        BOOST_TEST(dd5.size == sizeof(data5));
        BOOST_CHECK(std::memcmp(dd5.data.get(), data5, dd5.size) == 0);

        const auto dd6 = ds.read(addr6);
        BOOST_TEST(dd6.size == sizeof(data6));
        BOOST_CHECK(std::memcmp(dd6.data.get(), data6, dd6.size) == 0);

        const auto dd7 = ds.read(addr7);
        BOOST_TEST(dd7.size == sizeof(data7));
        BOOST_CHECK(std::memcmp(dd7.data.get(), data7, dd7.size) == 0);

        const auto dd8 = ds.read(addr8);
        BOOST_TEST(dd8.size == sizeof(data8));
        BOOST_CHECK(std::memcmp(dd8.data.get(), data8, dd8.size) == 0);

        const auto dd10 = ds.read(addr10);
        BOOST_TEST(dd10.size == sizeof(data10));
        BOOST_CHECK(std::memcmp(dd10.data.get(), data10, dd10.size) == 0);

        BOOST_TEST(ds.get_used_space() == expected_size);

        ds.sync();

    }

    {
        chaining_data_store ds (make_chaining_data_store_config());
        BOOST_TEST(ds.get_used_space() == expected_size);
        const auto d1 = ds.read(addr1);
        BOOST_TEST(d1.size == sizeof(data1));
        BOOST_CHECK(std::memcmp(d1.data.get(), data1, d1.size) == 0);

        const auto d3 = ds.read(addr3);
        BOOST_TEST(d3.size == sizeof(data3));
        BOOST_CHECK(std::memcmp(d3.data.get(), data3, d3.size) == 0);

        const auto d4 = ds.read(addr4);
        BOOST_TEST(d4.size == sizeof(data4));
        BOOST_CHECK(std::memcmp(d4.data.get(), data4, d4.size) == 0);

        const auto d5 = ds.read(addr5);
        BOOST_TEST(d5.size == sizeof(data5));
        BOOST_CHECK(std::memcmp(d5.data.get(), data5, d5.size) == 0);

        const auto d6 = ds.read(addr6);
        BOOST_TEST(d6.size == sizeof(data6));
        BOOST_CHECK(std::memcmp(d6.data.get(), data6, d6.size) == 0);

        const auto d7 = ds.read(addr7);
        BOOST_TEST(d7.size == sizeof(data7));
        BOOST_CHECK(std::memcmp(d7.data.get(), data7, d7.size) == 0);

        const auto d8 = ds.read(addr8);
        BOOST_TEST(d8.size == sizeof(data8));
        BOOST_CHECK(std::memcmp(d8.data.get(), data8, d8.size) == 0);


        BOOST_CHECK_THROW (ds.write(data11), std::bad_alloc);

    }
    cleanup();
}

// ---------------------------------------------------------------------

} // end namespace uh::cluster
