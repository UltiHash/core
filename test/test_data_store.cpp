#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "data_store tests"
#endif

#include "common/utils/random.h"
#include "common/utils/temp_directory.h"
#include "storage/data_store.h"
#include <boost/test/unit_test.hpp>
#include <random>

// ------------- Tests Suites Follow --------------

#define MAX_DATA_STORE_SIZE_BYTES (16 * 1024ul)
#define MAX_FILE_SIZE_BYTES (8 * 1024ul)
#define DATA_STORE_ID 1

namespace uh::cluster {

struct data_store_fixture {

    struct test_data {
        test_data() { fill_data(); }

        void fill_data() {
            std::memset(zero, 0, sizeof(zero));

            std::random_device rd;
            std::mt19937 generator(rd());
            std::uniform_int_distribution<> distribution(
                1, MAX_DATA_STORE_SIZE_BYTES + 1);

            size_t t_size = 0;
            while (t_size < MAX_DATA_STORE_SIZE_BYTES) {
                size_t length = distribution(generator);
                std::string random_data = random_string(length);

                if (t_size + random_data.size() <= MAX_DATA_STORE_SIZE_BYTES) {
                    data.push_back(random_data);
                    t_size += random_data.size();
                } else {
                    throwing_data = random_data;
                    t_data_size = t_size;
                    break;
                }
            }
        }

        std::size_t used_size() const {
            std::size_t files_created =
                (t_data_size + MAX_FILE_SIZE_BYTES - 1) / MAX_FILE_SIZE_BYTES;
            return t_data_size + files_created * sizeof(size_t);
        }

        std::vector<std::string> data;
        std::size_t t_data_size;
        std::string throwing_data;
        char zero[MAX_DATA_STORE_SIZE_BYTES];
    };

    data_store_config make_data_store_config() const {
        return {.working_dir = m_dir.path().string(),
                .min_file_size = 1024ul,
                .max_file_size = MAX_FILE_SIZE_BYTES,
                .max_data_store_size = MAX_DATA_STORE_SIZE_BYTES};
    }

    auto make_data_store() const {
        return std::make_unique<data_store>(make_data_store_config(),
                                            DATA_STORE_ID);
    }

    void setup() { ds = make_data_store(); }

    inline address write(auto& data) const { return ds->write(data); }

    static inline size_t expected_file_size(size_t t_written) {
        auto files =
            (t_written + MAX_FILE_SIZE_BYTES - 1) / MAX_FILE_SIZE_BYTES;
        return files * sizeof(size_t);
    }

    inline std::vector<address> write_all() {
        std::vector<address> addresses;

        for (auto& data : test_data.data) {
            addresses.emplace_back(ds->write(data));
        }

        return addresses;
    }

    temp_directory m_dir;
    test_data test_data;
    std::unique_ptr<data_store> ds;
};

BOOST_FIXTURE_TEST_SUITE(data_store_test_suite, data_store_fixture)

BOOST_AUTO_TEST_CASE(test_used_and_available_space) {
    size_t t_written = 0;

    for (auto& data : test_data.data) {
        write(data);
        t_written += data.size();

        auto used_size = t_written + expected_file_size(t_written);

        BOOST_TEST(ds->get_used_space() == used_size);
        BOOST_TEST(ds->get_available_space() ==
                   MAX_DATA_STORE_SIZE_BYTES - used_size);
    }
}

BOOST_AUTO_TEST_CASE(test_write) {
    write_all();
    BOOST_TEST(ds->get_used_space() == test_data.used_size());
    BOOST_CHECK_THROW(ds->write(test_data.throwing_data), std::bad_alloc);
}

BOOST_AUTO_TEST_CASE(test_read) {
    char buf[MAX_DATA_STORE_SIZE_BYTES];

    BOOST_CHECK_THROW(
        ds->read(buf, (DATA_STORE_ID + 1) * MAX_DATA_STORE_SIZE_BYTES, 1),
        std::out_of_range);
    BOOST_CHECK_THROW(
        ds->read(buf, DATA_STORE_ID * MAX_DATA_STORE_SIZE_BYTES - 1, 1),
        std::out_of_range);

    for (auto& data : test_data.data) {
        auto address = write(data);

        size_t t_read = 0;
        for (size_t i = 0; i < address.size(); i++) {
            const auto p = address.get_fragment(i);
            auto read_size = ds->read(buf + t_read, p.pointer, p.size);
            t_read += read_size;
        }

        BOOST_TEST(t_read == data.size());
        BOOST_CHECK(std::memcmp(buf, data.data(), t_read) == 0);
    }
}

BOOST_AUTO_TEST_CASE(test_remove) {

    BOOST_CHECK_THROW(
        ds->remove((DATA_STORE_ID + 1) * MAX_DATA_STORE_SIZE_BYTES, 1),
        std::out_of_range);
    BOOST_CHECK_THROW(
        ds->remove(DATA_STORE_ID * MAX_DATA_STORE_SIZE_BYTES - 1, 1),
        std::out_of_range);

    char buf[MAX_DATA_STORE_SIZE_BYTES];

    auto addresses = write_all();
    size_t removed_size = 0;
    size_t iteration = 0;

    for (auto& address : addresses) {
        for (size_t i = 0; i < address.size(); i++) {
            ds->remove(address.get_fragment(i).pointer,
                       address.get_fragment(i).size);
        }

        size_t t_read = 0;
        for (size_t i = 0; i < address.size(); ++i) {
            const auto p = address.get_fragment(i);
            auto read_size = ds->read(buf + t_read, p.pointer, p.size);
            t_read += read_size;
        }

        BOOST_TEST(t_read == test_data.data[iteration].size());
        BOOST_CHECK(std::memcmp(buf, test_data.zero, t_read) == 0);

        removed_size += t_read;
        BOOST_CHECK(ds->get_used_space() ==
                    test_data.used_size() - removed_size);

        iteration++;
    }

    // FOUND ISSUE : files not removed if everything is deleted and is 0
}

BOOST_AUTO_TEST_CASE(test_sync) {
    const std::size_t RND_ELEM = rand() % (test_data.data.size());
    auto address = write_all()[RND_ELEM];
    ds->sync();
    ds.reset();

    ds = make_data_store();

    BOOST_TEST(ds->get_used_space() == test_data.used_size());
    BOOST_TEST(ds->get_available_space() ==
               MAX_DATA_STORE_SIZE_BYTES - test_data.used_size());

    BOOST_CHECK_THROW(ds->write(test_data.throwing_data), std::bad_alloc);

    char buf[MAX_DATA_STORE_SIZE_BYTES];
    auto read = [&]() {
        size_t t_read = 0;

        for (size_t i = 0; i < address.size(); ++i) {
            const auto p = address.get_fragment(i);
            auto read_size = ds->read(buf + t_read, p.pointer, p.size);
            t_read += read_size;
        }

        return t_read;
    };

    auto t_read = read();
    BOOST_CHECK(t_read == test_data.data[RND_ELEM].size());
    BOOST_CHECK(std::memcmp(buf, test_data.data[RND_ELEM].data(), t_read) == 0);

    for (size_t i = 0; i < address.size(); i++) {
        ds->remove(address.get_fragment(i).pointer,
                   address.get_fragment(i).size);
    }

    t_read = read();
    BOOST_TEST(t_read == test_data.data[RND_ELEM].size());
    BOOST_CHECK(std::memcmp(buf, test_data.zero, t_read) == 0);

    BOOST_CHECK(ds->get_used_space() == test_data.used_size() - t_read);
    BOOST_CHECK(ds->get_available_space() ==
                MAX_DATA_STORE_SIZE_BYTES - test_data.used_size() + t_read);
}

BOOST_AUTO_TEST_SUITE_END()

} // end namespace uh::cluster
