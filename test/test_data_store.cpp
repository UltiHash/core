#define BOOST_TEST_MODULE "data_store tests"

#include "common/types/common_types.h"
#include "common/utils/random.h"
#include "common/utils/temp_directory.h"
#include "storage/data_store.h"
#include <boost/test/unit_test.hpp>
#include <random>
#include <thread>

// ------------- Tests Suites Follow --------------

#define MAX_DATA_STORE_SIZE_BYTES (4 * MEBI_BYTE)
#define MAX_FILE_SIZE_BYTES (128 * KIBI_BYTE)
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
            test_data.emplace_back(random_buffer(length));
            t_size = get_expected_used(length);
        }
        throwing_data = random_buffer(length);
        m_expected_used = 0;
        m_expected_last_file_space = 0;
    }

    [[nodiscard]] data_store_config make_data_store_config() const {
        return {.file_size = MAX_FILE_SIZE_BYTES,
                .max_data_store_size = MAX_DATA_STORE_SIZE_BYTES,
                .page_size = DEFAULT_PAGE_SIZE};
    }

    [[nodiscard]] auto make_data_store() const {
        return std::make_unique<data_store>(
            make_data_store_config(), m_dir.path().string(), DATA_STORE_ID, 0);
    }

    void setup() {
        ds = make_data_store();
        fill_data();
    }

    inline size_t get_expected_used(size_t t_written) {
        if (t_written > m_expected_last_file_space) {
            m_expected_last_file_space = MAX_FILE_SIZE_BYTES;
        }
        m_expected_used += t_written;
        m_expected_last_file_space -= t_written;
        return m_expected_used;
    }

    temp_directory m_dir;
    std::vector<shared_buffer<char>> test_data;
    shared_buffer<char> throwing_data;

    std::unique_ptr<data_store> ds;
    std::size_t m_expected_used{};
    std::size_t m_expected_last_file_space{};
};

BOOST_FIXTURE_TEST_SUITE(data_store_test_suite, data_store_fixture)

BOOST_AUTO_TEST_CASE(test_used_and_available_space) {

    long failures = 0;

    for (auto& data : test_data) {
        ds->write(data.string_view());

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
        auto address = ds->write(data.string_view());
        size_t t_read = 0;
        for (size_t i = 0; i < address.size(); i++) {
            const auto p = address.get(i);
            auto read_size = ds->read(buf + t_read, p.pointer, p.size);
            t_read += read_size;
        }

        if (t_read != data.size()) {
            failures++;
        }
        if (std::memcmp(buf, data.data(), t_read) != 0) {
            failures++;
        }
    }
    BOOST_TEST(failures == 0);
}

BOOST_AUTO_TEST_CASE(test_sync) {
    const std::size_t RND_ELEM = rand() % (test_data.size());

    std::vector<address> addresses;
    for (auto& data : test_data) {
        addresses.emplace_back(ds->write(data.string_view()));
    }
    auto address = addresses[RND_ELEM];
    ds.reset();

    ds = make_data_store();

    BOOST_CHECK_THROW(ds->write(throwing_data.string_view()), std::bad_alloc);

    char buf[MAX_FILE_SIZE_BYTES];
    size_t t_read = 0;

    for (size_t i = 0; i < address.size(); ++i) {
        const auto p = address.get(i);
        auto read_size = ds->read(buf + t_read, p.pointer, p.size);
        t_read += read_size;
    }

    BOOST_CHECK(t_read == test_data[RND_ELEM].size());
    BOOST_CHECK(std::memcmp(buf, test_data[RND_ELEM].data(), t_read) == 0);
}

BOOST_AUTO_TEST_CASE(stress_test) {
    size_t thread_count = 100;
    int thread_io_count = test_data.size() / thread_count;
    std::vector<std::thread> threads;
    threads.reserve(thread_count);
    std::atomic<size_t> failures = 0;
    std::exception_ptr eptr;
    for (size_t i = 0; i < thread_count; ++i) {
        threads.emplace_back([&, thread_id = i]() {
            try {
                std::vector<address> addresses;
                addresses.reserve(test_data.size());
                auto limit = std::min((thread_id + 1) * thread_io_count,
                                      test_data.size());
                for (size_t k = thread_id * thread_io_count; k < limit; ++k) {
                    addresses.emplace_back(ds->write(test_data[k].string_view()));
                }
                char buf[MAX_FILE_SIZE_BYTES];
                for (size_t j = 0; j < addresses.size(); ++j) {
                    auto f = addresses[j].get(0);
                    auto read_size = ds->read(buf, f.pointer, f.size);

                    if ((read_size !=
                         test_data[thread_id * thread_io_count + j].size())) {
                        failures++;
                    }
                    if (std::memcmp(
                            buf,
                            test_data[thread_id * thread_io_count + j].data(),
                            f.size) != 0) {
                        failures++;
                    }
                }
            } catch (const std::exception& e) {
                eptr = std::current_exception();
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    if (eptr) {
        std::rethrow_exception(eptr);
    }

    BOOST_TEST(failures == 0);
}

BOOST_AUTO_TEST_CASE(test_async_write) {

    auto read_address_compare = [this](const address& addr, const auto& data) {
        char buf[MAX_FILE_SIZE_BYTES];

        size_t t_read = 0;
        int failures = 0;
        for (size_t i = 0; i < addr.size(); i++) {
            const auto p = addr.get(i);
            auto read_size = ds->read(buf + t_read, p.pointer, p.size);
            t_read += read_size;
        }

        if (t_read != data.size()) {
            failures++;
        }
        if (std::memcmp(buf, data.data(), t_read) != 0) {
            failures++;
        }
        return failures;
    };

    long failures = 0;

    std::vector<address> addresses;
    for (auto& data : test_data) {
        addresses.emplace_back(ds->write(data.string_view()));
        failures += read_address_compare(addresses.back(), data);
    }

    for (size_t i = 0; i < test_data.size(); ++i) {
        failures += read_address_compare(addresses[i], test_data[i]);
    }

    for (size_t i = 0; i < test_data.size(); ++i) {
        failures += read_address_compare(addresses[i], test_data[i]);
    }

    BOOST_TEST(failures == 0);
}

BOOST_AUTO_TEST_CASE(test_link_unlink_invariant) {
    auto buffer = random_buffer(2 * DEFAULT_PAGE_SIZE);
    auto addr = ds->write(buffer.string_view());
    BOOST_CHECK_EQUAL(ds->unlink(addr), addr.data_size());
    addr = ds->write(buffer.string_view());
    BOOST_TEST(addr.size() == 1);
    address illegal_addr;
    illegal_addr.push({0, addr.data_size()/2});
    illegal_addr.push({addr.data_size()/2, (addr.data_size() - addr.data_size()/2)});
    BOOST_TEST(addr.data_size() == illegal_addr.data_size());
    BOOST_TEST(addr.size() != illegal_addr.size());
    BOOST_CHECK_NE(ds->unlink(illegal_addr), illegal_addr.data_size());
}

BOOST_AUTO_TEST_CASE(test_unlink_page_aligned) {
    auto buffer1 = random_buffer(DEFAULT_PAGE_SIZE);
    auto buffer2 = random_buffer(DEFAULT_PAGE_SIZE);
    auto buffer3 = random_buffer(2 * DEFAULT_PAGE_SIZE);

    address full_address;
    auto buffer1_address = ds->write(buffer1.string_view());
    auto buffer2_address = ds->write(buffer2.string_view());
    auto buffer3_address = ds->write(buffer3.string_view());
    full_address.append(buffer1_address);
    full_address.append(buffer2_address);
    full_address.append(buffer3_address);
    ds.reset();

    ds = make_data_store();

    {
        shared_buffer<char> read_buffer(full_address.data_size());
        size_t t_read = 0;
        for (size_t i = 0; i < full_address.size(); ++i) {
            const auto p = full_address.get(i);
            auto read_size =
                ds->read(read_buffer.data() + t_read, p.pointer, p.size);
            t_read += read_size;
        }

        BOOST_CHECK(t_read == full_address.data_size());
        BOOST_CHECK(std::memcmp(read_buffer.data(), buffer1.data(),
                                buffer1.size()) == 0);
        BOOST_CHECK(std::memcmp(read_buffer.data() + buffer1.size(),
                                buffer2.data(), buffer2.size()) == 0);
        BOOST_CHECK(
            std::memcmp(read_buffer.data() + buffer1.size() + buffer2.size(),
                        buffer3.data(), buffer3.size()) == 0);
    }

    ds->unlink(buffer2_address);

    {
        shared_buffer<char> read_buffer(full_address.data_size());
        shared_buffer<char> zero_buffer(buffer2.size());
        memset(zero_buffer.data(), 0, buffer2.size());
        size_t t_read = 0;
        for (size_t i = 0; i < full_address.size(); ++i) {
            const auto p = full_address.get(i);
            auto read_size =
                ds->read(read_buffer.data() + t_read, p.pointer, p.size);
            t_read += read_size;
        }

        BOOST_CHECK(t_read == full_address.data_size());
        BOOST_CHECK(std::memcmp(read_buffer.data(), buffer1.data(),
                                buffer1.size()) == 0);
        BOOST_CHECK(std::memcmp(read_buffer.data() + buffer1.size(),
                                zero_buffer.data(), zero_buffer.size()) == 0);
        BOOST_CHECK(std::memcmp(read_buffer.data() + buffer1.size() +
                                    zero_buffer.size(),
                                buffer3.data(), buffer3.size()) == 0);
    }
}

BOOST_AUTO_TEST_CASE(test_unlink_page_unaligned) {
    const std::size_t ALIGNMENT_OFFSET = 1337;
    auto buffer1 = random_buffer(DEFAULT_PAGE_SIZE + ALIGNMENT_OFFSET);
    auto buffer2 = random_buffer(2 * DEFAULT_PAGE_SIZE);
    auto buffer3 = random_buffer(DEFAULT_PAGE_SIZE - ALIGNMENT_OFFSET);

    address full_address;
    auto buffer1_address = ds->write(buffer1.string_view());
    auto buffer2_address = ds->write(buffer2.string_view());
    auto buffer3_address = ds->write(buffer3.string_view());
    full_address.append(buffer1_address);
    full_address.append(buffer2_address);
    full_address.append(buffer3_address);
    ds.reset();

    ds = make_data_store();

    {
        shared_buffer<char> read_buffer(full_address.data_size());
        size_t t_read = 0;
        for (size_t i = 0; i < full_address.size(); ++i) {
            const auto p = full_address.get(i);
            auto read_size =
                ds->read(read_buffer.data() + t_read, p.pointer, p.size);
            t_read += read_size;
        }

        BOOST_CHECK(t_read == full_address.data_size());
        BOOST_CHECK(std::memcmp(read_buffer.data(), buffer1.data(),
                                buffer1.size()) == 0);
        BOOST_CHECK(std::memcmp(read_buffer.data() + buffer1.size(),
                                buffer2.data(), buffer2.size()) == 0);
        BOOST_CHECK(
            std::memcmp(read_buffer.data() + buffer1.size() + buffer2.size(),
                        buffer3.data(), buffer3.size()) == 0);
    }

    ds->unlink(buffer2_address);

    {
        shared_buffer<char> read_buffer(full_address.data_size());
        shared_buffer<char> zero_buffer(DEFAULT_PAGE_SIZE);
        memset(zero_buffer.data(), 0, DEFAULT_PAGE_SIZE);
        size_t t_read = 0;
        for (size_t i = 0; i < full_address.size(); ++i) {
            const auto p = full_address.get(i);
            auto read_size =
                ds->read(read_buffer.data() + t_read, p.pointer, p.size);
            t_read += read_size;
        }

        BOOST_CHECK(t_read == full_address.data_size());
        BOOST_CHECK(std::memcmp(read_buffer.data(), buffer1.data(),
                                buffer1.size()) == 0);
        BOOST_CHECK(std::memcmp(read_buffer.data() + buffer1.size(),
                                buffer2.data(),
                                DEFAULT_PAGE_SIZE - ALIGNMENT_OFFSET) == 0);
        BOOST_CHECK(std::memcmp(read_buffer.data() + 2 * DEFAULT_PAGE_SIZE,
                                zero_buffer.data(), DEFAULT_PAGE_SIZE) == 0);
        BOOST_CHECK(
            std::memcmp(read_buffer.data() + 3 * DEFAULT_PAGE_SIZE,
                        buffer2.data() + buffer2.size() - ALIGNMENT_OFFSET,
                        ALIGNMENT_OFFSET) == 0);
        BOOST_CHECK(std::memcmp(read_buffer.data() + 3 * DEFAULT_PAGE_SIZE +
                                    ALIGNMENT_OFFSET,
                                buffer3.data(),
                                DEFAULT_PAGE_SIZE - ALIGNMENT_OFFSET) == 0);
    }
}

BOOST_AUTO_TEST_CASE(test_unlink_multi_file) {
    auto buffer1 = random_buffer(MAX_FILE_SIZE_BYTES - 1337);
    auto buffer2 = random_buffer(2 * DEFAULT_PAGE_SIZE);

    address full_address;
    auto buffer1_address = ds->write(buffer1.string_view());
    auto buffer2_address = ds->write(buffer2.string_view());
    full_address.append(buffer1_address);
    full_address.append(buffer2_address);
    ds.reset();

    ds = make_data_store();

    {
        shared_buffer<char> read_buffer(full_address.data_size());
        size_t t_read = 0;
        for (size_t i = 0; i < full_address.size(); ++i) {
            const auto p = full_address.get(i);
            auto read_size =
                ds->read(read_buffer.data() + t_read, p.pointer, p.size);
            t_read += read_size;
        }

        BOOST_CHECK(t_read == full_address.data_size());
        BOOST_CHECK(std::memcmp(read_buffer.data(), buffer1.data(),
                                buffer1.size()) == 0);
        BOOST_CHECK(std::memcmp(read_buffer.data() + buffer1.size(),
                                buffer2.data(), buffer2.size()) == 0);
    }

    ds->unlink(buffer2_address);

    {
        shared_buffer<char> read_buffer(full_address.data_size());
        shared_buffer<char> zero_buffer(buffer2.size() - 1337);
        memset(zero_buffer.data(), 0, zero_buffer.size());
        size_t t_read = 0;
        for (size_t i = 0; i < full_address.size(); ++i) {
            const auto p = full_address.get(i);
            auto read_size =
                ds->read(read_buffer.data() + t_read, p.pointer, p.size);
            t_read += read_size;
        }

        BOOST_CHECK(t_read == full_address.data_size());

        BOOST_CHECK(std::memcmp(read_buffer.data(), buffer1.data(),
                                buffer1.size()) == 0);
        BOOST_CHECK(std::memcmp(read_buffer.data() + buffer1.size(),
                                buffer2.data(), 1337) == 0);
        BOOST_CHECK(std::memcmp(read_buffer.data() + MAX_FILE_SIZE_BYTES,
                                zero_buffer.data(), zero_buffer.size()) == 0);
        BOOST_CHECK(buffer1.size() + 1337 + zero_buffer.size() == t_read);
    }
}

BOOST_AUTO_TEST_CASE(repeated_write_delete) {
    auto buffer = random_buffer(MAX_FILE_SIZE_BYTES / 4);

    address buffer_address;
    for (std::size_t i = 0; i < 100; i++) {
        buffer_address = ds->write(buffer.string_view());
        ds->unlink(buffer_address);
    }

    buffer_address = ds->write(buffer.string_view());

    shared_buffer<char> read_buffer(buffer_address.data_size());
    size_t t_read = 0;
    for (size_t i = 0; i < buffer_address.size(); ++i) {
        const auto p = buffer_address.get(i);
        auto read_size =
            ds->read(read_buffer.data() + t_read, p.pointer, p.size);
        t_read += read_size;
    }

    BOOST_CHECK(t_read == buffer_address.data_size());
    BOOST_CHECK(std::memcmp(read_buffer.data(), buffer.data(), buffer.size()) ==
                0);
}

BOOST_AUTO_TEST_SUITE_END()

} // end namespace uh::cluster
