#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "data_store tests"
#endif

#include "common/utils/common.h"
#include "common/utils/temp_directory.h"
#include "storage/data_store.h"
#include <boost/test/unit_test.hpp>

// ------------- Tests Suites Follow --------------

namespace uh::cluster {

struct data_store_fixture {
    static data_store_config make_data_store_config() {
        return {
            .working_dir = m_dir.path().string(),
            .min_file_size = 1024ul,
            .max_file_size = 8ul * 1024ul,
            .max_data_store_size = 16 * 1024ul,
        };
    }

    static auto get_data_store() {
        return std::make_unique<data_store>(make_data_store_config(), 0);
    }

private:
    static temp_directory m_dir;
};
temp_directory data_store_fixture::m_dir = {};

struct test_data {
    test_data() {
        fill_random(data1, sizeof(data1));
        fill_random(data2, sizeof(data2));
        fill_random(data3, sizeof(data3));
        fill_random(data4, sizeof(data4));
        fill_random(data5, sizeof(data5));
        fill_random(data6, sizeof(data6));
        fill_random(data7, sizeof(data7));
        fill_random(data8, sizeof(data8));
        fill_random(data9, sizeof(data9));
        fill_random(data10, sizeof(data10));
        fill_random(data11, sizeof(data11));
        std::memset(zero, 0, sizeof(zero));
    }

    static void fill_random(char* buf, size_t size) {
        for (size_t i = 0; i < size; ++i) {
            buf[i] = rand() & 0xff;
        }
    }

    char data1[512];
    char data2[1024];
    char data3[164];
    char data4[1520];
    char data5[2572];
    char data6[3021];
    char data7[102];
    char data8[5021];
    char data9[2048];
    char data10[3202];
    char data11[2021];
    char buf[8 * 1024];
    char zero[8 * 1024];
};

BOOST_AUTO_TEST_SUITE(data_store_test_suite)

test_data test;

address addr1, addr2, addr3, addr4, addr5, addr6, addr7, addr8, addr9, addr10,
    addr11;
size_t expected_size = sizeof(size_t);

auto ds = data_store_fixture::get_data_store();

BOOST_AUTO_TEST_CASE(test_write) {
    addr1 = ds->write(test.data1);
    expected_size += sizeof(test.data1);
    BOOST_CHECK(ds->get_used_space() == expected_size);
    addr2 = ds->write(test.data2);
    expected_size += sizeof(test.data2);
    BOOST_CHECK(ds->get_used_space() == expected_size);
    addr3 = ds->write(test.data3);
    expected_size += sizeof(test.data3);
    BOOST_CHECK(ds->get_used_space() == expected_size);
    addr4 = ds->write(test.data4);
    expected_size += sizeof(test.data4);
    addr5 = ds->write(test.data5);
    expected_size += sizeof(test.data5);
    BOOST_CHECK(ds->get_used_space() == expected_size);
    addr6 = ds->write(test.data6);
    expected_size += sizeof(test.data6) + sizeof(std::size_t); // new file
    addr7 = ds->write(test.data7);
    expected_size += sizeof(test.data7);
    addr8 = ds->write(test.data8);
    expected_size += sizeof(test.data8);
    addr9 = ds->write(test.data9);
    expected_size += sizeof(test.data9);
    BOOST_CHECK(ds->get_used_space() == expected_size);

    BOOST_CHECK_THROW(ds->write(test.data10), std::bad_alloc);
    BOOST_CHECK(ds->get_used_space() == expected_size);
}

BOOST_AUTO_TEST_CASE(test_read) {
    size_t rsize;
    rsize = ds->read(test.buf, addr1.get_fragment(0).pointer,
                     addr1.get_fragment(0).size);
    BOOST_TEST(rsize == sizeof(test.data1));
    BOOST_CHECK(std::memcmp(test.buf, test.data1, rsize) == 0);

    rsize = ds->read(test.buf, addr2.get_fragment(0).pointer,
                     addr2.get_fragment(0).size);
    BOOST_TEST(rsize == sizeof(test.data2));
    BOOST_CHECK(std::memcmp(test.buf, test.data2, rsize) == 0);

    rsize = ds->read(test.buf, addr3.get_fragment(0).pointer,
                     addr3.get_fragment(0).size);
    BOOST_TEST(rsize == sizeof(test.data3));
    BOOST_CHECK(std::memcmp(test.buf, test.data3, rsize) == 0);

    rsize = ds->read(test.buf, addr4.get_fragment(0).pointer,
                     addr4.get_fragment(0).size);
    BOOST_TEST(rsize == sizeof(test.data4));
    BOOST_CHECK(std::memcmp(test.buf, test.data4, rsize) == 0);

    rsize = ds->read(test.buf, addr5.get_fragment(0).pointer,
                     addr5.get_fragment(0).size);
    BOOST_TEST(rsize == sizeof(test.data5));
    BOOST_CHECK(std::memcmp(test.buf, test.data5, rsize) == 0);
}

BOOST_AUTO_TEST_CASE(test_total_read) {
    size_t rsize;
    size_t ts = 0;

    for (size_t i = 0; i < addr6.size(); ++i) {
        const auto p = addr6.get_fragment(i);
        rsize = ds->read(test.buf + ts, p.pointer, p.size);
        ts += rsize;
    }
    BOOST_TEST(ts == sizeof(test.data6));
    BOOST_CHECK(std::memcmp(test.buf, test.data6, ts) == 0);

    ts = 0;
    for (size_t i = 0; i < addr7.size(); ++i) {
        const auto p = addr7.get_fragment(i);
        rsize = ds->read(test.buf + ts, p.pointer, p.size);
        ts += rsize;
    }
    BOOST_TEST(ts == sizeof(test.data7));
    BOOST_CHECK(std::memcmp(test.buf, test.data7, ts) == 0);

    ts = 0;
    for (size_t i = 0; i < addr8.size(); ++i) {
        const auto p = addr8.get_fragment(i);
        rsize = ds->read(test.buf + ts, p.pointer, p.size);
        ts += rsize;
    }
    BOOST_TEST(ts == sizeof(test.data8));
    BOOST_CHECK(std::memcmp(test.buf, test.data8, ts) == 0);

    ts = 0;
    for (size_t i = 0; i < addr9.size(); ++i) {
        const auto p = addr9.get_fragment(i);
        rsize = ds->read(test.buf + ts, p.pointer, p.size);
        ts += rsize;
    }
    BOOST_TEST(ts == sizeof(test.data9));
    BOOST_CHECK(std::memcmp(test.buf, test.data9, ts) == 0);

    BOOST_CHECK(ds->get_used_space() == expected_size);
}

BOOST_AUTO_TEST_CASE(test_remove) {
    size_t ts = 0;
    size_t rsize;

    ds->remove(addr9.get_fragment(0).pointer, addr9.get_fragment(0).size);

    for (size_t i = 0; i < addr9.size(); ++i) {
        const auto p = addr9.get_fragment(i);
        rsize = ds->read(test.buf + ts, p.pointer, p.size);
        ts += rsize;
    }
    expected_size -= ts;

    BOOST_TEST(ts == sizeof(test.data9));
    BOOST_CHECK(std::memcmp(test.buf, test.zero, ts) == 0);

    BOOST_CHECK_THROW(ds->write(test.data10), std::bad_alloc);
    BOOST_CHECK(ds->get_used_space() == expected_size);

    ds->remove(addr2.get_fragment(0).pointer, addr2.get_fragment(0).size);
    ts = 0;
    for (size_t i = 0; i < addr2.size(); ++i) {
        const auto p = addr2.get_fragment(i);
        rsize = ds->read(test.buf + ts, p.pointer, p.size);
        ts += rsize;
    }
    expected_size -= ts;

    BOOST_TEST(ts == sizeof(test.data2));
    BOOST_CHECK(std::memcmp(test.buf, test.zero, ts) == 0);

    addr10 = ds->write(test.data10);
    expected_size += sizeof(test.data10);
    BOOST_CHECK(ds->get_used_space() == expected_size);

    BOOST_CHECK_THROW(ds->write(test.data11), std::bad_alloc);
}

BOOST_AUTO_TEST_CASE(test_read_after_removal) {
    size_t ts = 0;
    size_t rsize;
    for (size_t i = 0; i < addr6.size(); ++i) {
        const auto p = addr6.get_fragment(i);
        rsize = ds->read(test.buf + ts, p.pointer, p.size);
        ts += rsize;
    }

    BOOST_TEST(ts == sizeof(test.data6));
    BOOST_CHECK(std::memcmp(test.buf, test.data6, ts) == 0);

    ts = 0;
    for (size_t i = 0; i < addr7.size(); ++i) {
        const auto p = addr7.get_fragment(i);
        rsize = ds->read(test.buf + ts, p.pointer, p.size);
        ts += rsize;
    }
    BOOST_TEST(ts == sizeof(test.data7));
    BOOST_CHECK(std::memcmp(test.buf, test.data7, ts) == 0);

    ts = 0;
    for (size_t i = 0; i < addr8.size(); ++i) {
        const auto p = addr8.get_fragment(i);
        rsize = ds->read(test.buf + ts, p.pointer, p.size);
        ts += rsize;
    }
    BOOST_TEST(ts == sizeof(test.data8));
    BOOST_CHECK(std::memcmp(test.buf, test.data8, ts) == 0);

    BOOST_CHECK(ds->get_used_space() == expected_size);
}

BOOST_AUTO_TEST_CASE(test_persistence) {
    ds->sync();
    ds.reset();

    ds = data_store_fixture::get_data_store();
    BOOST_TEST(ds->get_used_space().get_data()[1] == expected_size);

    size_t ts, rsize;

    BOOST_CHECK_THROW(ds->write(test.data11), std::bad_alloc);

    ts = 0;
    for (size_t i = 0; i < addr6.size(); ++i) {
        const auto p = addr6.get_fragment(i);
        rsize = ds->read(test.buf + ts, p.pointer, p.size);
        ts += rsize;
    }
    BOOST_TEST(ts == sizeof(test.data6));
    BOOST_CHECK(std::memcmp(test.buf, test.data6, ts) == 0);

    ts = 0;
    for (size_t i = 0; i < addr7.size(); ++i) {
        const auto p = addr7.get_fragment(i);
        rsize = ds->read(test.buf + ts, p.pointer, p.size);
        ts += rsize;
    }
    BOOST_TEST(ts == sizeof(test.data7));
    BOOST_CHECK(std::memcmp(test.buf, test.data7, ts) == 0);

    ts = 0;
    for (size_t i = 0; i < addr8.size(); ++i) {
        const auto p = addr8.get_fragment(i);
        rsize = ds->read(test.buf + ts, p.pointer, p.size);
        ts += rsize;
    }
    BOOST_TEST(ts == sizeof(test.data8));
    BOOST_CHECK(std::memcmp(test.buf, test.data8, ts) == 0);

    ts = 0;
    for (size_t i = 0; i < addr6.size(); ++i) {
        const auto p = addr6.get_fragment(i);
        ds->remove(p.pointer, p.size);
        ts += p.size;
    }
    expected_size -= ts;
    BOOST_CHECK(ds->get_used_space() == expected_size);

    addr11 = ds->write(test.data11);
    expected_size += sizeof(test.data11);
    BOOST_CHECK(ds->get_used_space() == expected_size);

    ts = 0;
    for (size_t i = 0; i < addr11.size(); ++i) {
        const auto p = addr11.get_fragment(i);
        rsize = ds->read(test.buf + ts, p.pointer, p.size);
        ts += rsize;
    }
    BOOST_TEST(ts == sizeof(test.data11));
    BOOST_CHECK(std::memcmp(test.buf, test.data11, ts) == 0);
}

BOOST_AUTO_TEST_SUITE_END()

} // end namespace uh::cluster
