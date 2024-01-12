//
// Created by masi on 7/24/23.
//

#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "directory_store tests"
#endif

#include <boost/test/unit_test.hpp>
#include <directory/directory_store.h>

// ------------- Tests Suites Follow --------------

namespace uh::cluster {

// ---------------------------------------------------------------------

struct config_fixture
{
    static uh::cluster::bucket_config make_bucket_config () {
        return {
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

void fill_random_dirstore (char* buf, size_t size) {
    for (int i = 0; i < size; ++i) {
        buf[i] = rand()&0xff;
    }
}

// ---------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE (directory_store_test, config_fixture)

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

    fill_random_dirstore (data1, sizeof (data1));
    fill_random_dirstore (data2, sizeof (data2));
    fill_random_dirstore (data3, sizeof (data3));
    fill_random_dirstore (data4, sizeof (data4));
    fill_random_dirstore (data5, sizeof (data5));
    fill_random_dirstore (data6, sizeof (data6));
    fill_random_dirstore (data7, sizeof (data7));
    fill_random_dirstore (data8, sizeof (data8));
    fill_random_dirstore (data9, sizeof (data9));
    fill_random_dirstore (data10, sizeof (data10));
    fill_random_dirstore (data11, sizeof (data11));

    {
        directory_store ds("root");
        BOOST_CHECK_THROW (ds.insert("b1", "k1", data1), error_exception);
        ds.add_bucket("b1");
        ds.insert("b1", "k1", data1);
        ds.insert("b1", "k2", data2);
        ds.insert("b1", "k3", data3);
        ds.add_bucket("b2");
        ds.insert("b2", "k3", data3);
        ds.insert("b2", "k4", data4);
        ds.insert("b2", "k5", data5);
        ds.insert("b2", "k6", data6);
        ds.add_bucket("b3");
        ds.insert("b3", "k7", data7);
        ds.insert("b3", "k5", data5);
        ds.insert("b3", "k8", data8);

        const auto buckets = ds.list_buckets();
        BOOST_TEST (buckets.size() == 3);
        BOOST_CHECK (std::find (buckets.begin(), buckets.end(), "b1") != buckets.end());
        BOOST_CHECK (std::find (buckets.begin(), buckets.end(), "b2") != buckets.end());
        BOOST_CHECK (std::find (buckets.begin(), buckets.end(), "b3") != buckets.end());

        const auto b1 = ds.list_keys ("b1");
        BOOST_TEST (b1.size() == 3);
        BOOST_CHECK (std::find (b1.begin(), b1.end(), "k1") != b1.end());
        BOOST_CHECK (std::find (b1.begin(), b1.end(), "k2") != b1.end());
        BOOST_CHECK (std::find (b1.begin(), b1.end(), "k3") != b1.end());

        const auto b2= ds.list_keys ("b2");
        BOOST_TEST (b2.size() == 4);
        BOOST_CHECK (std::find (b2.begin(), b2.end(), "k3") != b2.end());
        BOOST_CHECK (std::find (b2.begin(), b2.end(), "k4") != b2.end());
        BOOST_CHECK (std::find (b2.begin(), b2.end(), "k5") != b2.end());
        BOOST_CHECK (std::find (b2.begin(), b2.end(), "k6") != b2.end());

        const auto b3 = ds.list_keys ("b3");
        BOOST_TEST (b3.size() == 3);
        BOOST_CHECK (std::find (b3.begin(), b3.end(), "k7") != b3.end());
        BOOST_CHECK (std::find (b3.begin(), b3.end(), "k5") != b3.end());
        BOOST_CHECK (std::find (b3.begin(), b3.end(), "k8") != b3.end());

        const auto d1 = ds.get("b1", "k1");
        BOOST_TEST(d1.size() == sizeof(data1));
        BOOST_CHECK(std::memcmp(d1.data(), data1, d1.size()) == 0);

        const auto d2 = ds.get("b1", "k2");
        BOOST_TEST(d2.size() == sizeof(data2));
        BOOST_CHECK(std::memcmp(d2.data(), data2, d2.size()) == 0);

        const auto d3 = ds.get("b1", "k3");
        BOOST_TEST(d3.size() == sizeof(data3));
        BOOST_CHECK(std::memcmp(d3.data(), data3, d3.size()) == 0);

        const auto d32 = ds.get("b2", "k3");
        BOOST_TEST(d32.size() == sizeof(data3));
        BOOST_CHECK(std::memcmp(d32.data(), data3, d32.size()) == 0);

        const auto d4 = ds.get("b2", "k4");
        BOOST_TEST(d4.size() == sizeof(data4));
        BOOST_CHECK(std::memcmp(d4.data(), data4, d4.size()) == 0);

        const auto d5 = ds.get("b2", "k5");
        BOOST_TEST(d5.size() == sizeof(data5));
        BOOST_CHECK(std::memcmp(d5.data(), data5, d5.size()) == 0);

        const auto d6 = ds.get("b2", "k6");
        BOOST_TEST(d6.size() == sizeof(data6));
        BOOST_CHECK(std::memcmp(d6.data(), data6, d6.size()) == 0);

        const auto d7 = ds.get("b3", "k7");
        BOOST_TEST(d7.size() == sizeof(data7));
        BOOST_CHECK(std::memcmp(d7.data(), data7, d7.size()) == 0);

        const auto d8 = ds.get("b3", "k8");
        BOOST_TEST(d8.size() == sizeof(data8));
        BOOST_CHECK(std::memcmp(d8.data(), data8, d8.size()) == 0);

        const auto d53 = ds.get("b3", "k5");
        BOOST_TEST(d53.size() == sizeof(data5));
        BOOST_CHECK(std::memcmp(d53.data(), data5, d5.size()) == 0);

    }

    {
        directory_store ds("root");
        const auto buckets = ds.list_buckets();
        BOOST_TEST (buckets.size() == 3);
        BOOST_CHECK (std::find (buckets.begin(), buckets.end(), "b1") != buckets.end());
        BOOST_CHECK (std::find (buckets.begin(), buckets.end(), "b2") != buckets.end());
        BOOST_CHECK (std::find (buckets.begin(), buckets.end(), "b3") != buckets.end());

        const auto b1 = ds.list_keys ("b1");
        BOOST_TEST (b1.size() == 3);
        BOOST_CHECK (std::find (b1.begin(), b1.end(), "k1") != b1.end());
        BOOST_CHECK (std::find (b1.begin(), b1.end(), "k2") != b1.end());
        BOOST_CHECK (std::find (b1.begin(), b1.end(), "k3") != b1.end());

        const auto b2= ds.list_keys ("b2");
        BOOST_TEST (b2.size() == 4);
        BOOST_CHECK (std::find (b2.begin(), b2.end(), "k3") != b2.end());
        BOOST_CHECK (std::find (b2.begin(), b2.end(), "k4") != b2.end());
        BOOST_CHECK (std::find (b2.begin(), b2.end(), "k5") != b2.end());
        BOOST_CHECK (std::find (b2.begin(), b2.end(), "k6") != b2.end());

        const auto b3 = ds.list_keys ("b3");
        BOOST_TEST (b3.size() == 3);
        BOOST_CHECK (std::find (b3.begin(), b3.end(), "k7") != b3.end());
        BOOST_CHECK (std::find (b3.begin(), b3.end(), "k5") != b3.end());
        BOOST_CHECK (std::find (b3.begin(), b3.end(), "k8") != b3.end());

        const auto d1 = ds.get("b1", "k1");
        BOOST_TEST(d1.size() == sizeof(data1));
        BOOST_CHECK(std::memcmp(d1.data(), data1, d1.size()) == 0);

        const auto d2 = ds.get("b1", "k2");
        BOOST_TEST(d2.size() == sizeof(data2));
        BOOST_CHECK(std::memcmp(d2.data(), data2, d2.size()) == 0);

        const auto d3 = ds.get("b1", "k3");
        BOOST_TEST(d3.size() == sizeof(data3));
        BOOST_CHECK(std::memcmp(d3.data(), data3, d3.size()) == 0);

        const auto d32 = ds.get("b2", "k3");
        BOOST_TEST(d32.size() == sizeof(data3));
        BOOST_CHECK(std::memcmp(d32.data(), data3, d32.size()) == 0);

        const auto d4 = ds.get("b2", "k4");
        BOOST_TEST(d4.size() == sizeof(data4));
        BOOST_CHECK(std::memcmp(d4.data(), data4, d4.size()) == 0);

        const auto d5 = ds.get("b2", "k5");
        BOOST_TEST(d5.size() == sizeof(data5));
        BOOST_CHECK(std::memcmp(d5.data(), data5, d5.size()) == 0);

        const auto d6 = ds.get("b2", "k6");
        BOOST_TEST(d6.size() == sizeof(data6));
        BOOST_CHECK(std::memcmp(d6.data(), data6, d6.size()) == 0);

        const auto d7 = ds.get("b3", "k7");
        BOOST_TEST(d7.size() == sizeof(data7));
        BOOST_CHECK(std::memcmp(d7.data(), data7, d7.size()) == 0);

        const auto d8 = ds.get("b3", "k8");
        BOOST_TEST(d8.size() == sizeof(data8));
        BOOST_CHECK(std::memcmp(d8.data(), data8, d8.size()) == 0);

        const auto d53 = ds.get("b3", "k5");
        BOOST_TEST(d53.size() == sizeof(data5));
        BOOST_CHECK(std::memcmp(d53.data(), data5, d5.size()) == 0);

        const auto used_space_1 = ds.get_used_space();

        ds.remove("b1", "k3");
        BOOST_CHECK_THROW (ds.get("b1", "k3"), std::exception);
        ds.remove("b2", "k4");
        BOOST_CHECK_THROW (ds.get("b2", "k4"), std::exception);

        const auto used_space_2 = ds.get_used_space();

        BOOST_CHECK (used_space_1 >= (used_space_2 + sizeof (data3) + sizeof (data4)));


    }

    {
        directory_store ds("root");
        const auto buckets = ds.list_buckets();
        BOOST_TEST (buckets.size() == 3);
        BOOST_CHECK (std::find (buckets.begin(), buckets.end(), "b1") != buckets.end());
        BOOST_CHECK (std::find (buckets.begin(), buckets.end(), "b2") != buckets.end());
        BOOST_CHECK (std::find (buckets.begin(), buckets.end(), "b3") != buckets.end());

        const auto b1 = ds.list_keys ("b1");
        BOOST_TEST (b1.size() == 2);
        BOOST_CHECK (std::find (b1.begin(), b1.end(), "k1") != b1.end());
        BOOST_CHECK (std::find (b1.begin(), b1.end(), "k2") != b1.end());

        const auto b2= ds.list_keys ("b2");
        BOOST_TEST (b2.size() == 3);
        BOOST_CHECK (std::find (b2.begin(), b2.end(), "k3") != b2.end());
        BOOST_CHECK (std::find (b2.begin(), b2.end(), "k5") != b2.end());
        BOOST_CHECK (std::find (b2.begin(), b2.end(), "k6") != b2.end());

        const auto b3 = ds.list_keys ("b3");
        BOOST_TEST (b3.size() == 3);
        BOOST_CHECK (std::find (b3.begin(), b3.end(), "k7") != b3.end());
        BOOST_CHECK (std::find (b3.begin(), b3.end(), "k5") != b3.end());
        BOOST_CHECK (std::find (b3.begin(), b3.end(), "k8") != b3.end());

        const auto d1 = ds.get("b1", "k1");
        BOOST_TEST(d1.size() == sizeof(data1));
        BOOST_CHECK(std::memcmp(d1.data(), data1, d1.size()) == 0);

        const auto d2 = ds.get("b1", "k2");
        BOOST_TEST(d2.size() == sizeof(data2));
        BOOST_CHECK(std::memcmp(d2.data(), data2, d2.size()) == 0);

        const auto d32 = ds.get("b2", "k3");
        BOOST_TEST(d32.size() == sizeof(data3));
        BOOST_CHECK(std::memcmp(d32.data(), data3, d32.size()) == 0);

        const auto d5 = ds.get("b2", "k5");
        BOOST_TEST(d5.size() == sizeof(data5));
        BOOST_CHECK(std::memcmp(d5.data(), data5, d5.size()) == 0);

        const auto d6 = ds.get("b2", "k6");
        BOOST_TEST(d6.size() == sizeof(data6));
        BOOST_CHECK(std::memcmp(d6.data(), data6, d6.size()) == 0);

        const auto d7 = ds.get("b3", "k7");
        BOOST_TEST(d7.size() == sizeof(data7));
        BOOST_CHECK(std::memcmp(d7.data(), data7, d7.size()) == 0);

        const auto d8 = ds.get("b3", "k8");
        BOOST_TEST(d8.size() == sizeof(data8));
        BOOST_CHECK(std::memcmp(d8.data(), data8, d8.size()) == 0);

        const auto d53 = ds.get("b3", "k5");
        BOOST_TEST(d53.size() == sizeof(data5));
        BOOST_CHECK(std::memcmp(d53.data(), data5, d5.size()) == 0);

        ds.remove_bucket("b2");
        BOOST_CHECK(ds.list_buckets().size() == 2);
    }

    {
        directory_store ds("root");
        const auto buckets = ds.list_buckets();
        //!! changed to 3 from 2 below
        BOOST_TEST (buckets.size() == 2);
        BOOST_CHECK (std::find (buckets.begin(), buckets.end(), "b1") != buckets.end());
        BOOST_CHECK (std::find (buckets.begin(), buckets.end(), "b3") != buckets.end());

    }

    cleanup();
}

// ---------------------------------------------------------------------

} // end namespace uh::cluster
