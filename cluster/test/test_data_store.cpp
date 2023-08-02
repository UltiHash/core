//
// Created by masi on 7/24/23.
//
#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "data_store tests"
#endif

#include <boost/test/unit_test.hpp>
#include "common.h"
#include "free_spot_manager.h"
#include "data_store.h"

// ------------- Tests Suites Follow --------------

namespace uh::cluster {

// ---------------------------------------------------------------------

struct config_fixture
{
    static uh::cluster::data_store_config make_data_store_config () {
        return {
                .directory = "root/dn",
                .hole_log = "root/dn/log",
                .min_file_size = 1024ul,
                .max_file_size = 8ul * 1024ul,
                .max_data_store_size = 16 * 1024ul
        };
    }

    void cleanup () {
        std::filesystem::remove_all("root");
    }
};

// ---------------------------------------------------------------------

BOOST_AUTO_TEST_CASE (test_big_int)
{
    constexpr uint128_t b1 {0, 16140901064495857661ul};
    constexpr uint64_t b2 = 13835058055282163710ul;
    constexpr big_int res1 = b1 * b2; // 223310303291865866574052609832259682310
    constexpr uint8_t answer1 [] = {0xfc, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xa7,
                         0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    BOOST_TEST(std::memcmp (res1.get_data(), answer1, sizeof(answer1)) == 0);

    constexpr uint128_t b3 {0, 9213123140901423261ul};
    constexpr uint64_t b4 = 12332180552821123214ul;
    constexpr big_int res2 = b3 * b4; // 113617898028970796972859371597246680854
    constexpr uint8_t answer2 [] = {0xe1, 0x7f, 0x0f, 0x37, 0xde, 0x02, 0x7a, 0x55,
                                    0x16, 0xcf, 0x22, 0xd3, 0x35, 0xd3, 0x2d, 0xe9};
    BOOST_TEST(std::memcmp (res2.get_data(), answer2, sizeof(answer2)) == 0);

    constexpr auto res3 = res1 + res2; // 336928201320836663546911981429506363164
    constexpr uint8_t answer3 [] = {0xdd, 0x7f, 0x0f, 0x37, 0xde, 0x02, 0x7a, 0xfd,
                                    0x1c, 0xcf, 0x22, 0xd3, 0x35, 0xd3, 0x2d, 0xe9};
    BOOST_TEST(std::memcmp (res3.get_data(), answer3, sizeof(answer3)) == 0);


    constexpr auto res4 = res3 - res2;
    BOOST_TEST(std::memcmp (res4.get_data(), res1.get_data(), sizeof(res1)) == 0);

    BOOST_CHECK(res4 < res3);
    BOOST_CHECK(res3 > res2);
    BOOST_CHECK(res4 == res1);

}
// ---------------------------------------------------------------------

BOOST_AUTO_TEST_CASE (test_free_spot_manager)
{
    std::filesystem::remove ("__test_free_spot_manager");
    std::array <uint128_t, 100> offsets;
    std::array <std::size_t , 100> sizes {};

    for (auto& offset: offsets) {
        const auto n0 = rand () % std::numeric_limits <uint64_t>::max();
        const auto n1 = rand () % std::numeric_limits <uint64_t>::max();
        offset = {n0, n1};
    }

    for (auto& size: sizes) {
        size = {rand () % std::numeric_limits <uint64_t>::max()};
    }

    std::size_t expected_total_free_size = 0;
    {
        free_spot_manager fsm("__test_free_spot_manager");
        const auto free_spot_size1 = fsm.total_free_spots();
        BOOST_CHECK((free_spot_size1 == big_int {0, 0}));

        const auto fs1 = fsm.pop_free_spot();
        BOOST_CHECK(fs1 == std::nullopt);
        fsm.apply_popped_items();
        const auto free_spot_size2 = fsm.total_free_spots();

        BOOST_CHECK((free_spot_size2 == big_int {0, 0}));
        const auto fs2 = fsm.pop_free_spot();
        BOOST_CHECK(fs2 == std::nullopt);


        for (int i = 0; i < offsets.size(); ++i) {
            fsm.push_free_spot(offsets[i], sizes[i]);
            expected_total_free_size += sizes[i];

            const auto free_spot_size3 = fsm.total_free_spots();
            BOOST_CHECK((free_spot_size3 == big_int {0, expected_total_free_size}));
        }

        for (int i = 0; i < 20; ++i) {
            const auto fs = fsm.pop_free_spot();
            BOOST_CHECK (fs.value().pointer == offsets.at(i));
            BOOST_CHECK (fs.value().size == sizes.at(i));
            expected_total_free_size -= fs.value().size;

        }
        fsm.apply_popped_items();
        for (int i = 20; i < 40; ++i) {
            const auto fs = fsm.pop_free_spot();
            BOOST_CHECK (fs.value().pointer == offsets.at(i));
            BOOST_CHECK (fs.value().size == sizes.at(i));
        }

        const auto free_spot_size3 = fsm.total_free_spots();
        BOOST_CHECK((free_spot_size3 == big_int {0, expected_total_free_size}));

    }

    {

        free_spot_manager fsm("__test_free_spot_manager");
        const auto free_spot_size1 = fsm.total_free_spots();
        BOOST_CHECK((free_spot_size1 == big_int {0, expected_total_free_size}));

        for (int i = 20; i < 40; ++i) {
            const auto fs = fsm.pop_free_spot();
            BOOST_CHECK (fs.value().pointer == offsets.at(i));
            BOOST_CHECK (fs.value().size == sizes.at(i));
            expected_total_free_size -= fs.value().size;

        }

        fsm.apply_popped_items();

        const auto free_spot_size2 = fsm.total_free_spots();
        BOOST_CHECK((free_spot_size2 == big_int {0, expected_total_free_size}));

        for (int i = 0; i < 20; ++i) {
            fsm.push_free_spot(offsets[i], sizes[i]);
            expected_total_free_size += sizes[i];

            const auto free_spot_size3 = fsm.total_free_spots();
            BOOST_CHECK((free_spot_size3 == big_int {0, expected_total_free_size}));
        }

        for (int i = 40; i < 60; ++i) {
            const auto fs = fsm.pop_free_spot();
            BOOST_CHECK (fs.value().pointer == offsets.at(i));
            BOOST_CHECK (fs.value().size == sizes.at(i));
            expected_total_free_size -= fs.value().size;

        }
        fsm.apply_popped_items();

        const auto free_spot_size4 = fsm.total_free_spots();
        BOOST_CHECK((free_spot_size4 == big_int {0, expected_total_free_size}));

        for (int i = 60; i < 100; ++i) {
            const auto fs = fsm.pop_free_spot();
            BOOST_CHECK (fs.value().pointer == offsets.at(i));
            BOOST_CHECK (fs.value().size == sizes.at(i));
            expected_total_free_size -= fs.value().size;

        }
        for (int i = 0; i < 20; ++i) {
            const auto fs = fsm.pop_free_spot();
            BOOST_CHECK (fs.value().pointer == offsets.at(i));
            BOOST_CHECK (fs.value().size == sizes.at(i));
            expected_total_free_size -= fs.value().size;

        }
        fsm.apply_popped_items();
        const auto free_spot_size5 = fsm.total_free_spots();
        BOOST_CHECK((free_spot_size5 == big_int {0, 0}));

        const auto fs1 = fsm.pop_free_spot();
        BOOST_CHECK(fs1 == std::nullopt);

        for (int i = 0; i < 20; ++i) {
            fsm.push_free_spot(offsets[i], sizes[i]);
            expected_total_free_size += sizes[i];

            const auto free_spot_size3 = fsm.total_free_spots();
            BOOST_CHECK((free_spot_size3 == big_int {0, expected_total_free_size}));
        }

        for (int i = 0; i < 20; ++i) {
            const auto fs = fsm.pop_free_spot();
            BOOST_CHECK (fs.value().pointer == offsets.at(i));
            BOOST_CHECK (fs.value().size == sizes.at(i));
            expected_total_free_size -= fs.value().size;

        }
        fsm.apply_popped_items();
        const auto free_spot_size6 = fsm.total_free_spots();
        BOOST_CHECK((free_spot_size6 == big_int {0, 0}));
        const auto free_spot_size7 = fsm.total_free_spots();
        BOOST_CHECK((free_spot_size7 == big_int {0, 0}));

        const auto fs2 = fsm.pop_free_spot();
        BOOST_CHECK(fs2 == std::nullopt);
    }

    std::filesystem::remove ("__test_free_spot_manager");
}

// ---------------------------------------------------------------------

void fill_random(char* buf, size_t size) {
    for (int i = 0; i < size; ++i) {
        buf[i] = rand()&0xff;
    }
}

BOOST_FIXTURE_TEST_CASE (test_data_store, config_fixture)
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

    fill_random (data1, sizeof (data1));
    fill_random (data2, sizeof (data2));
    fill_random (data3, sizeof (data3));
    fill_random (data4, sizeof (data4));
    fill_random (data5, sizeof (data5));
    fill_random (data6, sizeof (data6));
    fill_random (data7, sizeof (data7));
    fill_random (data8, sizeof (data8));
    fill_random (data9, sizeof (data9));
    fill_random (data10, sizeof (data10));
    fill_random (data11, sizeof (data11));


    char buf [8*1024];
    char zero [8*1024];
    std::memset(zero, 0, sizeof(zero));

    address addr1, addr2, addr3, addr4, addr5, addr6, addr7, addr8, addr9, addr10, addr11;

    size_t expected_size = sizeof (size_t);
    {

        data_store ds (make_data_store_config(), 0);
        addr1 = ds.write(data1);
        expected_size += sizeof (data1);
        BOOST_CHECK(ds.get_used_space() == expected_size);
        addr2 = ds.write(data2);
        expected_size += sizeof (data2);
        BOOST_CHECK(ds.get_used_space() == expected_size);
        addr3 = ds.write(data3);
        expected_size += sizeof (data3);
        BOOST_CHECK(ds.get_used_space() == expected_size);
        addr4 = ds.write(data4);
        expected_size += sizeof (data4);
        addr5 = ds.write(data5);
        expected_size += sizeof (data5);
        BOOST_CHECK(ds.get_used_space() == expected_size);
        addr6 = ds.write(data6);
        expected_size += sizeof (data6) + sizeof (std::size_t); // new file
        addr7 = ds.write(data7);
        expected_size += sizeof (data7);
        addr8 = ds.write(data8);
        expected_size += sizeof (data8);
        addr9 = ds.write(data9);
        expected_size += sizeof (data9);
        BOOST_CHECK(ds.get_used_space() == expected_size);

        BOOST_CHECK_THROW (ds.write(data10), std::bad_alloc);
        BOOST_CHECK(ds.get_used_space() == expected_size);

        size_t rsize;
        rsize = ds.read(buf, addr1[0].pointer, addr1[0].size);
        BOOST_TEST(rsize == sizeof(data1));
        BOOST_CHECK(std::memcmp(buf, data1, rsize) == 0);

        rsize = ds.read(buf, addr2[0].pointer, addr2[0].size);
        BOOST_TEST(rsize == sizeof(data2));
        BOOST_CHECK(std::memcmp(buf, data2, rsize) == 0);

        rsize = ds.read(buf, addr3[0].pointer, addr3[0].size);
        BOOST_TEST(rsize == sizeof(data3));
        BOOST_CHECK(std::memcmp(buf, data3, rsize) == 0);

        rsize = ds.read(buf, addr4[0].pointer, addr4[0].size);
        BOOST_TEST(rsize == sizeof(data4));
        BOOST_CHECK(std::memcmp(buf, data4, rsize) == 0);

        rsize = ds.read(buf, addr5[0].pointer, addr5[0].size);
        BOOST_TEST(rsize == sizeof(data5));
        BOOST_CHECK(std::memcmp(buf, data5, rsize) == 0);

        size_t ts = 0;


        for (int i = 0; i < addr6.size (); ++i) {
            const auto p = addr6[i];
            rsize = ds.read(buf + ts, p.pointer, p.size);
            ts += rsize;
        }
        BOOST_TEST(ts == sizeof (data6));
        BOOST_CHECK(std::memcmp(buf, data6, ts) == 0);

        ts = 0;
        for (int i = 0; i < addr7.size (); ++i) {
            const auto p = addr7[i];
            rsize = ds.read(buf + ts, p.pointer, p.size);
            ts += rsize;
        }
        BOOST_TEST(ts == sizeof (data7));
        BOOST_CHECK(std::memcmp(buf, data7, ts) == 0);

        ts = 0;
        for (int i = 0; i < addr8.size (); ++i) {
            const auto p = addr8[i];
            rsize = ds.read(buf + ts, p.pointer, p.size);
            ts += rsize;
        }
        BOOST_TEST(ts == sizeof (data8));
        BOOST_CHECK(std::memcmp(buf, data8, ts) == 0);

        ts = 0;
        for (int i = 0; i < addr9.size (); ++i) {
            const auto p = addr9[i];
            rsize = ds.read(buf + ts, p.pointer, p.size);
            ts += rsize;
        }
        BOOST_TEST(ts == sizeof (data9));
        BOOST_CHECK(std::memcmp(buf, data9, ts) == 0);

        BOOST_CHECK(ds.get_used_space() == expected_size);

        ds.remove(addr9[0].pointer, addr9[0].size);
        ts = 0;
        for (int i = 0; i < addr9.size (); ++i) {
            const auto p = addr9[i];
            rsize = ds.read(buf + ts, p.pointer, p.size);
            ts += rsize;
        }
        expected_size -= ts;

        BOOST_TEST(ts == sizeof (data9));
        BOOST_CHECK(std::memcmp(buf, zero, ts) == 0);

        BOOST_CHECK_THROW (ds.write(data10), std::bad_alloc);
        BOOST_CHECK(ds.get_used_space() == expected_size);

        ds.remove(addr2[0].pointer, addr2[0].size);
        ts = 0;
        for (int i = 0; i < addr2.size (); ++i) {
            const auto p = addr2[i];
            rsize = ds.read(buf + ts, p.pointer, p.size);
            ts += rsize;
        }
        expected_size -= ts;

        BOOST_TEST(ts == sizeof (data2));
        BOOST_CHECK(std::memcmp(buf, zero, ts) == 0);

        const auto addr10 = ds.write(data10);
        expected_size += sizeof (data10);
        BOOST_CHECK(ds.get_used_space() == expected_size);

        BOOST_CHECK_THROW (ds.write(data11), std::bad_alloc);

        ts = 0;
        for (int i = 0; i < addr6.size (); ++i) {
            const auto p = addr6[i];
            rsize = ds.read(buf + ts, p.pointer, p.size);
            ts += rsize;
        }

        BOOST_TEST(ts == sizeof (data6));
        BOOST_CHECK(std::memcmp(buf, data6, ts) == 0);

        ts = 0;
        for (int i = 0; i < addr7.size (); ++i) {
            const auto p = addr7[i];
            rsize = ds.read(buf + ts, p.pointer, p.size);
            ts += rsize;
        }
        BOOST_TEST(ts == sizeof (data7));
        BOOST_CHECK(std::memcmp(buf, data7, ts) == 0);

        ts = 0;
        for (int i = 0; i < addr8.size (); ++i) {
            const auto p = addr8[i];
            rsize = ds.read(buf + ts, p.pointer, p.size);
            ts += rsize;
        }
        BOOST_TEST(ts == sizeof (data8));
        BOOST_CHECK(std::memcmp(buf, data8, ts) == 0);

        BOOST_CHECK(ds.get_used_space() == expected_size);

        ds.sync();

    }

    {
        data_store ds (make_data_store_config(), 0);
        BOOST_TEST(ds.get_used_space().get_data()[1] == expected_size);

        size_t ts, rsize;

        BOOST_CHECK_THROW (ds.write(data11), std::bad_alloc);

        ts = 0;

        for (int i = 0; i < addr6.size (); ++i) {
            const auto p = addr6[i];
            rsize = ds.read(buf + ts, p.pointer, p.size);
            ts += rsize;
        }
        BOOST_TEST(ts == sizeof (data6));
        BOOST_CHECK(std::memcmp(buf, data6, ts) == 0);

        ts = 0;
        for (int i = 0; i < addr7.size (); ++i) {
            const auto p = addr7[i];
            rsize = ds.read(buf + ts, p.pointer, p.size);
            ts += rsize;
        }
        BOOST_TEST(ts == sizeof (data7));
        BOOST_CHECK(std::memcmp(buf, data7, ts) == 0);

        ts = 0;
        for (int i = 0; i < addr8.size (); ++i) {
            const auto p = addr8[i];
            rsize = ds.read(buf + ts, p.pointer, p.size);
            ts += rsize;
        }
        BOOST_TEST(ts == sizeof (data8));
        BOOST_CHECK(std::memcmp(buf, data8, ts) == 0);

        ts = 0;
        for (int i = 0; i < addr6.size (); ++i) {
            const auto p = addr6[i];
            ds.remove(p.pointer, p.size);
            ts += p.size;
        }
        expected_size -= ts;
        BOOST_CHECK(ds.get_used_space() == expected_size);

        addr11 = ds.write(data11);
        expected_size += sizeof (data11);
        BOOST_CHECK(ds.get_used_space() == expected_size);

        ts = 0;

        for (int i = 0; i < addr11.size (); ++i) {
            const auto p = addr11[i];
            rsize = ds.read(buf + ts, p.pointer, p.size);
            ts += rsize;
        }
        BOOST_TEST(ts == sizeof (data11));
        BOOST_CHECK(std::memcmp(buf, data11, ts) == 0);
    }
    cleanup();
}

// ---------------------------------------------------------------------

} // end namespace uh::cluster