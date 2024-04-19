#define BOOST_TEST_MODULE "directory_store tests"

#include "common/utils/random.h"
#include "common/utils/temp_directory.h"
#include "directory/directory_store.h"
#include "storage/data_store.h"
#include <boost/test/unit_test.hpp>

// ------------- Tests Suites Follow --------------

namespace uh::cluster {

// ---------------------------------------------------------------------

struct directory_store_fixture {
    static uh::cluster::bucket_config make_bucket_config() {
        return {
            .min_file_size = 1024ul,
            .max_file_size = 8ul * 1024ul,
            .max_storage_size = 16 * 1024ul,
            .max_chunk_size = 16 * 1024ul,
        };
    }

    data_store_config make_data_store_config() const {
        return {.working_dir = m_dir.path().string(),
                .file_size = 8 * 1024ul,
                .max_data_store_size = 32 * 1024ul};
    }

    auto make_directory_store() const {
        return std::make_unique<directory_store>(
            directory_store_config(m_dir.path(), make_bucket_config()));
    }

    void setup() {
        ds = std::make_unique<data_store>(make_data_store_config(), 0, 0);
        dir_s = make_directory_store();

        data1 = random_buffer(512);
        data2 = random_buffer(1024);
        data3 = random_buffer(164);
        data4 = random_buffer(1520);
        data5 = random_buffer(2572);
        data6 = random_buffer(3021);
        data7 = random_buffer(102);
        data8 = random_buffer(5021);
        data9 = random_buffer(2048);
        data10 = random_buffer(3202);
        data11 = random_buffer(2021);

    }


    static bool find(const auto& key, const auto& objects) {
        for (const auto& obj : objects) {
            if (obj.name == key)
                return true;
        }
        return false;
    };

    static bool compare_address(const auto& addr1, const auto& addr2) {
        for (size_t iteration = 0; iteration < addr1.pointers.size();
             iteration++) {
            if (addr1.pointers.at(iteration) != addr2.pointers.at(iteration) ||
                addr1.sizes.at(iteration / 2) !=
                    addr2.sizes.at(iteration / 2)) {
                return false;
            }
        }
        return true;
    };

    void write_all() {
        dir_s->add_bucket("b1");
        dir_s->insert("b1", "k1", addr1 = ds->register_write(data1));
        dir_s->insert("b1", "k2", addr2 = ds->register_write(data2));
        dir_s->insert("b1", "k3", addr3 = ds->register_write(data3));
        dir_s->add_bucket("b2");
        dir_s->insert("b2", "k4", addr4 = ds->register_write(data4));
        dir_s->insert("b2", "k5", addr5 = ds->register_write(data5));
        dir_s->insert("b2", "k6", addr6 = ds->register_write(data6));
        dir_s->insert("b2", "k7", addr7 = ds->register_write(data7));
        dir_s->add_bucket("b3");
        dir_s->insert("b3", "k8", addr8 = ds->register_write(data8));
        dir_s->insert("b3", "k9", addr9 = ds->register_write(data9));
        dir_s->insert("b3", "k10", addr10 = ds->register_write(data10));
    }

    temp_directory m_dir;
    std::unique_ptr<data_store> ds;
    std::unique_ptr<directory_store> dir_s;

    shared_buffer <char> data1;
    shared_buffer <char> data2;
    shared_buffer <char> data3;
    shared_buffer <char> data4;
    shared_buffer <char> data5;
    shared_buffer <char> data6;
    shared_buffer <char> data7;
    shared_buffer <char> data8;
    shared_buffer <char> data9;
    shared_buffer <char> data10;
    shared_buffer <char> data11;

    address addr1, addr2, addr3, addr4, addr5, addr6, addr7, addr8, addr9,
        addr10;
};

BOOST_FIXTURE_TEST_SUITE(directory_store_test, directory_store_fixture)

BOOST_AUTO_TEST_CASE(test_insert) {
    BOOST_CHECK_THROW(dir_s->insert("b1", "k1", address()), error_exception);
    write_all();
    BOOST_CHECK_NO_THROW(dir_s->insert("b1", "k1", address()));
}

BOOST_AUTO_TEST_CASE(test_bucket_exists) {
    BOOST_CHECK_THROW(dir_s->bucket_exists(""), error_exception);
    write_all();
    BOOST_CHECK_NO_THROW(dir_s->bucket_exists("b1"));
}

BOOST_AUTO_TEST_CASE(test_add_bucket) {
    constexpr const char* non_existing_bucket = "non-existing";
    BOOST_CHECK_THROW(dir_s->bucket_exists(non_existing_bucket),
                      error_exception);
    dir_s->add_bucket(non_existing_bucket);
    BOOST_CHECK_NO_THROW(dir_s->bucket_exists(non_existing_bucket));
}

BOOST_AUTO_TEST_CASE(test_remove_bucket) {
    write_all();
    BOOST_CHECK_NO_THROW(dir_s->get("b1", "k1"));
    dir_s->remove_bucket("b1");
    BOOST_CHECK_THROW(dir_s->get("b1", "k1"), error_exception);
    BOOST_CHECK_THROW(dir_s->bucket_exists("b1"), error_exception);
}

BOOST_AUTO_TEST_CASE(test_list_buckets) {
    write_all();
    const auto buckets = dir_s->list_buckets();
    BOOST_TEST(buckets.size() == 3);
    BOOST_CHECK(std::find(buckets.begin(), buckets.end(), "b1") !=
                buckets.end());
    BOOST_CHECK(std::find(buckets.begin(), buckets.end(), "b2") !=
                buckets.end());
    BOOST_CHECK(std::find(buckets.begin(), buckets.end(), "b3") !=
                buckets.end());
}

BOOST_AUTO_TEST_CASE(test_list_objects) {
    write_all();

    const auto b1 = dir_s->list_objects("b1");
    BOOST_TEST(b1.size() == 3);
    BOOST_TEST(find("k1", b1));
    BOOST_TEST(find("k2", b1));
    BOOST_TEST(find("k3", b1));

    const auto b2 = dir_s->list_objects("b2");
    BOOST_TEST(b2.size() == 4);
    BOOST_TEST(find("k4", b2));
    BOOST_TEST(find("k5", b2));
    BOOST_TEST(find("k6", b2));
    BOOST_TEST(find("k7", b2));

    const auto b3 = dir_s->list_objects("b3");
    BOOST_TEST(b3.size() == 3);
    BOOST_TEST(find("k8", b3));
    BOOST_TEST(find("k9", b3));
    BOOST_TEST(find("k10", b3));
}

BOOST_AUTO_TEST_CASE(test_get_object) {
    write_all();

    const auto k1_addr = dir_s->get("b1", "k1");
    BOOST_CHECK(compare_address(addr1, k1_addr));

    const auto k6_addr = dir_s->get("b2", "k6");
    BOOST_CHECK(compare_address(addr6, k6_addr));

    const auto k7_addr = dir_s->get("b3", "k10");
    BOOST_CHECK(compare_address(addr10, k7_addr));

    BOOST_CHECK_THROW(dir_s->get("non-existing", "key"), error_exception);
}

BOOST_AUTO_TEST_CASE(test_remove_object) {
    write_all();
    dir_s->remove_object("b1", "k3");
    BOOST_CHECK_THROW(dir_s->get("b1", "k3"), error_exception);
    dir_s->remove_object("b2", "k4");
    BOOST_CHECK_THROW(dir_s->get("b2", "k4"), error_exception);

    BOOST_CHECK_THROW(dir_s->get("non-existing", "key"), error_exception);
}

BOOST_AUTO_TEST_CASE(test_persistence) {
    write_all();
    dir_s.reset();
    dir_s = make_directory_store();

    const auto buckets = dir_s->list_buckets();
    BOOST_TEST(buckets.size() == 3);

    BOOST_CHECK(std::find(buckets.begin(), buckets.end(), "b1") !=
                buckets.end());

    const auto b1 = dir_s->list_objects("b1");
    BOOST_TEST(b1.size() == 3);
    BOOST_TEST(find("k1", b1));

    const auto k1_addr = dir_s->get("b1", "k1");
    BOOST_CHECK(compare_address(addr1, k1_addr));
}

BOOST_AUTO_TEST_CASE(test_used_space) {
    write_all();
    const auto used_space_1 = dir_s->get_used_space();

    dir_s->remove_object("b1", "k3");
    dir_s->remove_object("b2", "k4");

    const auto used_space_2 = dir_s->get_used_space();

    BOOST_CHECK((used_space_2 < used_space_1));
}

BOOST_AUTO_TEST_SUITE_END()

} // end namespace uh::cluster
