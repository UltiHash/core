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

namespace uh::cluster {

struct data_store_fixture {
    data_store_config make_data_store_config() {
        return {.working_dir = m_dir.path().string(),
                .min_file_size = 1024ul,
                .max_file_size = MAX_FILE_SIZE_BYTES,
                .max_data_store_size = MAX_DATA_STORE_SIZE_BYTES};
    }

    void setup() {
        ds_ptr = std::make_unique<data_store>(make_data_store_config(), 0);
        for (auto& data : test_data.m_data) {
            ds_ptr->write(data);
        }
    }

    struct test_data {
        test_data() { fill_random_data(); }

        void fill_random_data() {
            std::random_device rd;
            std::mt19937 generator(rd());
            std::uniform_int_distribution<std::mt19937::result_type>
                distribution(1, MAX_DATA_STORE_SIZE_BYTES);

            size_t total_size = 0;

            while (total_size < MAX_DATA_STORE_SIZE_BYTES) {
                size_t length = distribution(generator);
                std::string random_data = random_string(length);

                if (total_size + random_data.size() <=
                    MAX_DATA_STORE_SIZE_BYTES) {
                    m_data.push_back(random_data);
                    total_size += random_data.size();
                } else {
                    exceeded_data = random_data;
                    std::size_t files_created =
                        (total_size + MAX_FILE_SIZE_BYTES - 1) /
                        MAX_FILE_SIZE_BYTES;
                    actual_size = total_size + files_created * sizeof(size_t);
                    break;
                }
            }
        }

        std::vector<std::string> m_data;
        std::string exceeded_data;
        std::size_t actual_size;
    };

    temp_directory m_dir;
    test_data test_data;
    std::unique_ptr<data_store> ds_ptr;
};

BOOST_FIXTURE_TEST_SUITE(data_store_test_suite, data_store_fixture)

BOOST_AUTO_TEST_CASE(test_used_and_available_space) {
    BOOST_TEST(ds_ptr->get_used_space() == test_data.actual_size);
    BOOST_TEST(ds_ptr->get_available_space() ==
               MAX_DATA_STORE_SIZE_BYTES - test_data.actual_size);
}

BOOST_AUTO_TEST_CASE(test_write) {
    BOOST_CHECK_THROW(ds_ptr->write(test_data.exceeded_data), std::bad_alloc);
}

BOOST_AUTO_TEST_CASE(test_read) {}

BOOST_AUTO_TEST_CASE(test_remove) {}

BOOST_AUTO_TEST_CASE(test_sync) {}

BOOST_AUTO_TEST_SUITE_END()

} // end namespace uh::cluster
