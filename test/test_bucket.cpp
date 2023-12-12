//
// Created by masi on 7/24/23.
//

#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "bucket tests"
#endif

#include <boost/test/unit_test.hpp>
#include <directory_node/bucket.h>

// ------------- Tests Suites Follow --------------

namespace uh::cluster {

// ---------------------------------------------------------------------

struct config_fixture
{
    static std::filesystem::path get_root_path() {
        return "_tmp_test/dr/";
    }

    static void cleanup () {
        std::filesystem::remove_all("_tmp_test");
    }

    static void setup_prefilled () {
        std::filesystem::create_directories(get_root_path() / "bucket1");
        chaining_data_store_config ds_conf = {
                .directory = get_root_path() / "bucket1",
                .free_spot_log = get_root_path() / "bucket1/free_spot_log",
                .min_file_size = 1024ul,
                .max_file_size = 8ul * 1024ul,
                .max_storage_size = 16 * 1024ul,
                .max_chunk_size = 16 * 1024u,
                };
        chaining_data_store ds (ds_conf);
        char data1[] = "Would be a shame if we lost this data!";
        auto addr1 = ds.write(data1);

        uh::cluster::transaction_log tl(get_root_path()/"bucket1/transaction_log");
        tl.append("key1", addr1, transaction_log::INSERT_START);
        tl.append("key1", addr1, transaction_log::INSERT_END);

    }

    static void setup_empty () {
        std::filesystem::create_directories(get_root_path() / "bucket1");
        chaining_data_store_config ds_conf = {
                .directory = get_root_path() / "bucket1",
                .free_spot_log = get_root_path() / "bucket1/free_spot_log",
                .min_file_size = 1024ul,
                .max_file_size = 8ul * 1024ul,
                .max_storage_size = 16 * 1024ul,
                .max_chunk_size = 16 * 1024u,
        };
    }

    static uh::cluster::bucket_config get_bucket_config() {
        return {
            .min_file_size = 1024ul,
            .max_file_size = 8ul * 1024ul,
            .max_storage_size = 16 * 1024ul,
            .max_chunk_size = 16 * 1024ul,
        };
    }
};

void fill_random_object(char* buf, size_t size) {
    for (int i = 0; i < size; ++i) {
        buf[i] = rand()&0xff;
    }
}

// ---------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE (bucket_prefilled_test, config_fixture)
{
    cleanup();
    setup_prefilled();

    uh::cluster::bucket b(get_root_path(), "bucket1", get_bucket_config());
    auto restored_value = b.get_obj("key1");
    char original_value[] = "Would be a shame if we lost this data!";
    BOOST_CHECK(std::memcmp(original_value, restored_value.data(), restored_value.size()) == 0);

    cleanup();
}

//BOOST_FIXTURE_TEST_CASE (test_bucket_operations, config_fixture)
//{
//    cleanup();
//    setup_empty();
//
//    char bucket_data1 [1024 * 4];
//    char bucket_data2 [1024 * 64];
//    char bucket_data3 [1024 * 512];
//    char bucket_data4 [1024 * 1024 * 4];
//    char bucket_data5 [1024 * 1024 * 128];
//    char bucket_data6 [1024 * 1024 * 1024];
//
//    fill_random_object (data1, sizeof (data1));
//    fill_random_object (data2, sizeof (data2));
//    fill_random_object (data3, sizeof (data3));
//    fill_random_object (data4, sizeof (data4));
//    fill_random_object (data5, sizeof (data5));
//    fill_random_object (data6, sizeof (data6));
//
//    uh::cluster::bucket b(get_root_path(), "bucket1", get_bucket_config());
//    BOOST_CHECK(!b.contains_object("obj1"));
//    BOOST_CHECK(!b.contains_object("obj2"));
//    BOOST_CHECK(!b.contains_object("obj3"));
//    BOOST_CHECK(!b.contains_object("obj4"));
//    BOOST_CHECK(!b.contains_object("obj5"));
//    BOOST_CHECK(!b.contains_object("obj6"));
//
//    b.insert_object("obj1", {data1, sizeof (data1)});
//    b.insert_object("obj2", {data2, sizeof (data2)});
//    b.insert_object("obj3", {data3, sizeof (data3)});
//    b.insert_object("obj4", {data4, sizeof (data4)});
//    b.insert_object("obj5", {data5, sizeof (data5)});
//    b.insert_object("obj6", {data6, sizeof (data6)});
//    BOOST_CHECK(b.contains_object("obj1"));
//    BOOST_CHECK(b.contains_object("obj2"));
//    BOOST_CHECK(b.contains_object("obj3"));
//    BOOST_CHECK(b.contains_object("obj4"));
//    BOOST_CHECK(b.contains_object("obj5"));
//    BOOST_CHECK(b.contains_object("obj6"));
//
//    auto ret1 = b.get_obj("obj1");
//    BOOST_CHECK(std::memcmp(data1, ret1.data.get(), ret1.size) == 0);
//    auto ret2 = b.get_obj("obj2");
//    BOOST_CHECK(std::memcmp(data2, ret2.data.get(), ret2.size) == 0);
//    auto ret3 = b.get_obj("obj3");
//    BOOST_CHECK(std::memcmp(data3, ret3.data.get(), ret3.size) == 0);
//    auto ret4 = b.get_obj("obj4");
//    BOOST_CHECK(std::memcmp(data4, ret4.data.get(), ret4.size) == 0);
//    auto ret5 = b.get_obj("obj5");
//    BOOST_CHECK(std::memcmp(data5, ret5.data.get(), ret5.size) == 0);
//    auto ret6 = b.get_obj("obj6");
//    BOOST_CHECK(std::memcmp(data6, ret6.data.get(), ret6.size) == 0);
//
//
//    cleanup();
//}

// ---------------------------------------------------------------------

} // end namespace uh::cluster
