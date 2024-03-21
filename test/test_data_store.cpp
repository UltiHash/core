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
#include <thread>

// ------------- Tests Suites Follow --------------

#define MAX_DATA_STORE_SIZE_BYTES (4 * 1024ul * 1024ul)
#define MAX_FILE_SIZE_BYTES (8 * 1024ul)
#define DATA_STORE_ID 1

namespace uh::cluster {

struct data_store_fixture {

    void fill_data() {
        std::random_device rd;
        std::mt19937 generator(rd());
        generator.seed(3);
        std::uniform_int_distribution<> distribution(
                1, MAX_FILE_SIZE_BYTES / 4 + 1);

        size_t length = distribution(generator);
        size_t t_size = get_expected_used(length);
        while (t_size < MAX_DATA_STORE_SIZE_BYTES) {
            test_data.emplace_back(random_string(length));
            t_size = get_expected_used(length);
        }
        throwing_data = random_string(length);
        m_expected_used = 0;
    }

    [[nodiscard]] data_store_config make_data_store_config() const {
        return {.working_dir = m_dir.path().string(),
                .file_size = MAX_FILE_SIZE_BYTES,
                .max_data_store_size = MAX_DATA_STORE_SIZE_BYTES};
    }

    [[nodiscard]] auto make_data_store() const {
        return std::make_unique<data_store>(make_data_store_config(),
                                            DATA_STORE_ID);
    }

    void setup() {
        ds = make_data_store();
        fill_data();
    }

    inline size_t get_expected_used(size_t t_written) {
        auto files =
                (m_expected_used + MAX_FILE_SIZE_BYTES - 1) / MAX_FILE_SIZE_BYTES;

        if (m_expected_used + t_written > files * MAX_FILE_SIZE_BYTES) {
            m_expected_used = files * MAX_FILE_SIZE_BYTES + sizeof (size_t);

        }
        else if (m_expected_used == 0) {
            m_expected_used = sizeof (size_t);
        }

        m_expected_used += t_written;
        return m_expected_used;
    }

    temp_directory m_dir;
    std::vector<std::string> test_data;
    std::string throwing_data;

    std::unique_ptr<data_store> ds;
    std::size_t m_expected_used{};
};

BOOST_FIXTURE_TEST_SUITE(data_store_test_suite, data_store_fixture)

BOOST_AUTO_TEST_CASE(test_used_and_available_space) {

    long failures = 0;

    for (auto& data : test_data) {
        ds->write(data);

        auto used_size = get_expected_used(data.size());

        if (ds->get_used_space() != used_size) {
            failures++;
        }
        if (ds->get_available_space() !=
            MAX_DATA_STORE_SIZE_BYTES - used_size) {
            failures++;
        }
    }
    BOOST_TEST(failures == 0);

}


BOOST_AUTO_TEST_CASE(test_read) {
    char buf[MAX_FILE_SIZE_BYTES];

    BOOST_CHECK_THROW(
        ds->read(buf, (DATA_STORE_ID + 1) * MAX_DATA_STORE_SIZE_BYTES, 1),
        std::out_of_range);
    BOOST_CHECK_THROW(
        ds->read(buf, DATA_STORE_ID * MAX_DATA_STORE_SIZE_BYTES - 1, 1),
        std::out_of_range);

    long failures = 0;
    for (auto& data : test_data) {
        auto address = ds->write(data);

        size_t t_read = 0;
        for (size_t i = 0; i < address.size(); i++) {
            const auto p = address.get_fragment(i);
            auto read_size = ds->read(buf + t_read, p.pointer, p.size);
            t_read += read_size;
        }

        if (t_read != data.size()) {
            failures ++;
        }
        if (std::memcmp(buf, data.data(), t_read) != 0) {
            failures ++;
        }
    }
    BOOST_TEST(failures == 0);
}

BOOST_AUTO_TEST_CASE(test_sync) {
    const std::size_t RND_ELEM = rand() % (test_data.size());

    std::vector<address> addresses;
    for (auto& data : test_data) {
        addresses.emplace_back(ds->write(data));
    }
    auto address = addresses[RND_ELEM];
    ds->sync();
    ds.reset();

    ds = make_data_store();

    BOOST_CHECK_THROW(ds->write(throwing_data), std::bad_alloc);

    char buf[MAX_FILE_SIZE_BYTES];
    size_t t_read = 0;

    for (size_t i = 0; i < address.size(); ++i) {
        const auto p = address.get_fragment(i);
        auto read_size = ds->read(buf + t_read, p.pointer, p.size);
        t_read += read_size;
    }

    BOOST_CHECK(t_read == test_data[RND_ELEM].size());
    BOOST_CHECK(std::memcmp(buf, test_data[RND_ELEM].data(), t_read) == 0);

}

BOOST_AUTO_TEST_CASE(stress_test) {
    size_t thread_count = 100;
    int thread_io_count = test_data.size() / thread_count;
    std::vector <std::thread> threads;
    threads.reserve(thread_count);
    std::atomic<size_t> failures = 0;
    std::exception_ptr eptr;
    for (size_t i = 0; i < thread_count; ++i) {
        threads.emplace_back([&, thread_id = i] () {
            try {
                std::vector <address> addresses;
                addresses.reserve(test_data.size());
                auto limit = std::min ((thread_id+1)*thread_io_count, test_data.size());
                for (size_t k = thread_id * thread_io_count; k < limit; ++k) {
                    addresses.emplace_back(ds->write(test_data[k]));
                }
                char buf [MAX_FILE_SIZE_BYTES];
                for (size_t j = 0; j < addresses.size(); ++j) {
                    auto f = addresses[j].get_fragment(0);
                    auto read_size = ds->read(buf, f.pointer, f.size);

                    if ((read_size != test_data[thread_id * thread_io_count + j].size())) {
                        failures ++;
                    }
                    if (std::memcmp(buf, test_data[thread_id * thread_io_count + j].data(), f.size) != 0) {
                        failures ++;
                    }
                }
            }
            catch (const std::exception& e) {
                eptr = std::current_exception();
            }
        });
    }

    for (auto& t: threads) {
        t.join();
    }

    if (eptr) {
        std::rethrow_exception(eptr);
    }

    BOOST_TEST(failures == 0);
}

BOOST_AUTO_TEST_SUITE_END()

} // end namespace uh::cluster
