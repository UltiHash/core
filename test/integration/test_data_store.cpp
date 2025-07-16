#define BOOST_TEST_MODULE "data_store tests"

#include <boost/test/unit_test.hpp>

#include <common/telemetry/log.h>
#include <common/types/common_types.h>
#include <common/utils/random.h>
#include <storage/default_data_store.h>

#include <util/random.h>
#include <util/temp_directory.h>

#include <common/utils/pointer_traits.h>
#include <random>
#include <thread>

// ------------- Tests Suites Follow --------------

#define MAX_DATA_STORE_SIZE_BYTES (4 * MEBI_BYTE)
#define MAX_FILE_SIZE_BYTES (128 * KIBI_BYTE)

namespace uh::cluster {

#define CHECK_EQUAL_FROM_OFFSET(read, offset, org)                             \
    BOOST_CHECK_EQUAL_COLLECTIONS((read).begin() + offset,                     \
                                  (read).begin() + offset + org.size(),        \
                                  (org).begin(), (org).end())

struct data_store_fixture {

    void fill_data() {
        std::random_device rd;
        std::mt19937 generator(rd());
        generator.seed(3);
        std::uniform_int_distribution<> distribution(
            1, MAX_FILE_SIZE_BYTES / 4 + 1);

        size_t length = distribution(generator);
        size_t t_size = get_expected_used(length);
        while (t_size < MAX_DATA_STORE_SIZE_BYTES / 2) {
            test_data.emplace_back(random_buffer(length));
            t_size = get_expected_used(length);
        }
        throwing_data = random_buffer(MAX_DATA_STORE_SIZE_BYTES);
        m_expected_used = 0;
        m_expected_last_file_space = 0;
    }

    [[nodiscard]] data_store_config make_data_store_config() const {
        return {.max_file_size = MAX_FILE_SIZE_BYTES,
                .max_data_store_size = MAX_DATA_STORE_SIZE_BYTES,
                .page_size = DEFAULT_PAGE_SIZE};
    }

    [[nodiscard]] auto make_data_store() const {
        return std::make_unique<default_data_store>(make_data_store_config(),
                                                    m_dir.path().string(), 0);
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

    std::vector<refcount_t> extract_refcounts(const address& addr) {
        std::map<std::size_t, std::size_t> refcount_by_stripe;

        for (const auto& frag : addr.fragments) {
            auto local_pointer =
                pointer_traits::get_group_pointer(frag.pointer);
            std::size_t first_stripe = local_pointer / DEFAULT_PAGE_SIZE;
            std::size_t last_stripe =
                (local_pointer + frag.size - 1) / DEFAULT_PAGE_SIZE;
            for (size_t stripe_id = first_stripe; stripe_id <= last_stripe;
                 stripe_id++) {
                refcount_by_stripe[stripe_id]++;
            }
        }

        std::vector<refcount_t> refcounts;
        refcounts.reserve(refcount_by_stripe.size());
        for (const auto& [stripe_id, count] : refcount_by_stripe) {
            refcounts.emplace_back(stripe_id, count);
        }
        return refcounts;
    }

    std::vector<refcount_t>
    make_default_refcounts(const allocation_t& allocation) {
        std::vector<refcount_t> derived_refcounts;
        std::size_t first_stripe = allocation.offset / DEFAULT_PAGE_SIZE;
        std::size_t last_stripe =
            (allocation.offset + allocation.size - 1) / DEFAULT_PAGE_SIZE;
        for (size_t stripe_id = first_stripe; stripe_id <= last_stripe;
             stripe_id++) {
            derived_refcounts.emplace_back(stripe_id, 1);
        }
        return derived_refcounts;
    }

    temp_directory m_dir;
    std::vector<shared_buffer<char>> test_data;
    shared_buffer<char> throwing_data;

    std::unique_ptr<default_data_store> ds;
    std::size_t m_expected_used{};
    std::size_t m_expected_last_file_space{};
};

BOOST_FIXTURE_TEST_SUITE(data_store_test_suite, data_store_fixture)

BOOST_AUTO_TEST_CASE(empty_data_store) {
    BOOST_CHECK_EQUAL(ds->get_used_space(), 0ull);
    BOOST_CHECK_EQUAL(ds->get_available_space(), MAX_DATA_STORE_SIZE_BYTES);
}

BOOST_AUTO_TEST_CASE(write_updates_space) {
    BOOST_CHECK_EQUAL(ds->get_used_space(), 0ull);
    BOOST_CHECK_EQUAL(ds->get_available_space(), MAX_DATA_STORE_SIZE_BYTES);

    auto data = random_buffer(1 * MEBI_BYTE);
    auto alloc = ds->allocate(data.size());
    ds->write(alloc, {data.string_view()}, make_default_refcounts(alloc));

    BOOST_CHECK_EQUAL(ds->get_used_space(), 1 * MEBI_BYTE);
    BOOST_CHECK_EQUAL(ds->get_available_space(),
                      MAX_DATA_STORE_SIZE_BYTES - 1 * MEBI_BYTE);
}

BOOST_AUTO_TEST_CASE(test_used_and_available_space) {

    long failures = 0;

    for (auto& data : test_data) {
        auto alloc = ds->allocate(data.size());
        ds->write(alloc, {data.string_view()}, make_default_refcounts(alloc));

        auto used_size = get_expected_used(data.size());
        BOOST_TEST(ds->get_used_space() == used_size);
        if (ds->get_used_space() != used_size) {
            failures++;
        }
        BOOST_TEST(ds->get_available_space() ==
                   MAX_DATA_STORE_SIZE_BYTES - used_size);
        if (ds->get_available_space() !=
            MAX_DATA_STORE_SIZE_BYTES - used_size) {
            failures++;
        }
    }
    BOOST_TEST(failures == 0);
}

BOOST_AUTO_TEST_CASE(test_read) {
    char buf[MAX_FILE_SIZE_BYTES];

    BOOST_CHECK_THROW(ds->read(2 * MAX_DATA_STORE_SIZE_BYTES, {buf, 1}),
                      std::out_of_range);
    BOOST_CHECK_THROW(ds->read(MAX_DATA_STORE_SIZE_BYTES - 1, {buf, 1}),
                      std::out_of_range);

    long failures = 0;
    for (auto& data : test_data) {
        auto alloc = ds->allocate(data.size());
        ds->write(alloc, {data.string_view()}, make_default_refcounts(alloc));
        address addr;
        addr.emplace_back(alloc.offset, data.size());
        size_t t_read = 0;
        for (size_t i = 0; i < addr.size(); i++) {
            const auto p = addr.get(i);
            auto read_size = ds->read(p.pointer, {buf + t_read, p.size});
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
        auto alloc = ds->allocate(data.size());
        ds->write(alloc, {data.string_view()}, make_default_refcounts(alloc));
        address addr;
        addr.emplace_back(alloc.offset, data.size());
        addresses.emplace_back(addr);
    }
    auto address = addresses[RND_ELEM];
    ds.reset();

    ds = make_data_store();

    BOOST_CHECK_THROW(ds->allocate(throwing_data.size()), std::exception);

    char buf[MAX_FILE_SIZE_BYTES];
    size_t t_read = 0;

    for (size_t i = 0; i < address.size(); ++i) {
        const auto p = address.get(i);
        auto read_size = ds->read(p.pointer, {buf + t_read, p.size});
        t_read += read_size;
    }

    BOOST_CHECK(t_read == test_data[RND_ELEM].size());
    BOOST_CHECK(std::memcmp(buf, test_data[RND_ELEM].data(), t_read) == 0);
}

BOOST_AUTO_TEST_CASE(stress_test) {
    size_t thread_count = test_data.size();
    std::vector<std::thread> threads;
    threads.reserve(thread_count);
    std::atomic<size_t> failures = 0;
    std::exception_ptr eptr;
    for (size_t i = 0; i < thread_count; ++i) {
        threads.emplace_back([&, thread_id = i]() {
            try {
                auto alloc = ds->allocate(test_data[thread_id].size());
                ds->write(alloc, {test_data[thread_id].string_view()},
                          make_default_refcounts(alloc));
                address addr;
                addr.emplace_back(alloc.offset, test_data[thread_id].size());
                char read_buf[MAX_FILE_SIZE_BYTES];

                size_t read_size = 0ull;
                for (unsigned id = 0; id < addr.size(); ++id) {
                    auto f = addr.get(id);
                    read_size +=
                        ds->read(f.pointer, {read_buf + read_size, f.size});
                }

                if ((read_size != test_data[thread_id].size())) {
                    LOG_DEBUG()
                        << "Read size mismatch: expected "
                        << test_data[thread_id].size() << ", got " << read_size;
                    failures++;
                }
                if (std::memcmp(read_buf, test_data[thread_id].data(),
                                read_size) != 0) {
                    LOG_DEBUG() << "Data mismatch for thread " << thread_id;
                    failures++;
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
            auto read_size = ds->read(p.pointer, {buf + t_read, p.size});
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
        auto alloc = ds->allocate(data.size());
        ds->write(alloc, {data.string_view()}, make_default_refcounts(alloc));
        address addr;
        addr.emplace_back(alloc.offset, data.size());
        addresses.emplace_back(addr);
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

    auto alloc = ds->allocate(buffer.size());
    ds->write(alloc, {buffer.string_view()}, make_default_refcounts(alloc));
    address addr;
    addr.emplace_back(alloc.offset, buffer.size());

    auto refcounts = extract_refcounts(addr);
    BOOST_CHECK_EQUAL(ds->unlink(refcounts), addr.data_size());

    auto alloc2 = ds->allocate(buffer.size());
    ds->write(alloc2, {buffer.string_view()}, make_default_refcounts(alloc2));
    addr = address{};
    addr.emplace_back(alloc2.offset, buffer.size());

    address illegal_addr;
    illegal_addr.push({0, addr.data_size() / 2});
    illegal_addr.push(
        {addr.data_size() / 2, (addr.data_size() - addr.data_size() / 2)});

    BOOST_TEST(addr.data_size() == illegal_addr.data_size());
    BOOST_TEST(addr.size() != illegal_addr.size());
    auto illegal_refcounts = extract_refcounts(illegal_addr);
    BOOST_CHECK_THROW(ds->unlink(illegal_refcounts), std::exception);
}

BOOST_AUTO_TEST_CASE(test_unlink_page_aligned) {
    auto buffer1 = random_buffer(DEFAULT_PAGE_SIZE);
    auto buffer2 = random_buffer(DEFAULT_PAGE_SIZE);
    auto buffer3 = random_buffer(2 * DEFAULT_PAGE_SIZE);

    address full_address;
    auto alloc1 = ds->allocate(buffer1.size());
    auto alloc2 = ds->allocate(buffer2.size());
    auto alloc3 = ds->allocate(buffer3.size());
    ds->write(alloc1, {buffer1.string_view()}, make_default_refcounts(alloc1));
    ds->write(alloc2, {buffer2.string_view()}, make_default_refcounts(alloc2));
    ds->write(alloc3, {buffer3.string_view()}, make_default_refcounts(alloc3));
    address buffer1_address;
    buffer1_address.emplace_back(alloc1.offset, buffer1.size());
    address buffer2_address;
    buffer2_address.emplace_back(alloc2.offset, buffer2.size());
    address buffer3_address;
    buffer3_address.emplace_back(alloc3.offset, buffer3.size());
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
                ds->read(p.pointer, {read_buffer.data() + t_read, p.size});
            t_read += read_size;
        }

        BOOST_CHECK(t_read == full_address.data_size());
        size_t offset = 0;
        CHECK_EQUAL_FROM_OFFSET(read_buffer, offset, buffer1);
        offset += buffer1.size();
        CHECK_EQUAL_FROM_OFFSET(read_buffer, offset, buffer2);
        offset += buffer2.size();
        CHECK_EQUAL_FROM_OFFSET(read_buffer, offset, buffer3);
        offset += buffer3.size();
    }

    auto buffer2_refcounts = extract_refcounts(buffer2_address);
    ds->unlink(buffer2_refcounts);

    {
        shared_buffer<char> read_buffer(full_address.data_size());
        size_t t_read = 0;
        for (size_t i = 0; i < full_address.size(); ++i) {
            const auto p = full_address.get(i);
            auto read_size =
                ds->read(p.pointer, {read_buffer.data() + t_read, p.size});
            t_read += read_size;
        }

        BOOST_CHECK(t_read == full_address.data_size());
        size_t offset = 0;
        CHECK_EQUAL_FROM_OFFSET(read_buffer, offset, buffer1);
        shared_buffer<char> zero_buffer(buffer2.size());
        memset(zero_buffer.data(), 0, buffer2.size());
        offset += buffer1.size();
        CHECK_EQUAL_FROM_OFFSET(read_buffer, offset, zero_buffer);
        offset += buffer2.size();
        CHECK_EQUAL_FROM_OFFSET(read_buffer, offset, buffer3);
        offset += buffer3.size();
    }
}

BOOST_AUTO_TEST_CASE(test_unlink_page_unaligned) {
    const std::size_t ALIGNMENT_OFFSET = 1337;
    auto buffer1 = random_buffer(DEFAULT_PAGE_SIZE + ALIGNMENT_OFFSET);
    auto buffer2 = random_buffer(2 * DEFAULT_PAGE_SIZE);
    auto buffer3 = random_buffer(DEFAULT_PAGE_SIZE - ALIGNMENT_OFFSET);

    address full_address;
    auto alloc1 = ds->allocate(buffer1.size(), 1);
    auto alloc2 = ds->allocate(buffer2.size(), 1);
    auto alloc3 = ds->allocate(buffer3.size(), 1);
    ds->write(alloc1, {buffer1.string_view()}, make_default_refcounts(alloc1));
    ds->write(alloc2, {buffer2.string_view()}, make_default_refcounts(alloc2));
    ds->write(alloc3, {buffer3.string_view()}, make_default_refcounts(alloc3));
    address buffer1_address;
    buffer1_address.emplace_back(alloc1.offset, buffer1.size());
    address buffer2_address;
    buffer2_address.emplace_back(alloc2.offset, buffer2.size());
    address buffer3_address;
    buffer3_address.emplace_back(alloc3.offset, buffer3.size());
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
                ds->read(p.pointer, {read_buffer.data() + t_read, p.size});
            t_read += read_size;
        }

        BOOST_CHECK(t_read == full_address.data_size());
        size_t offset = 0;
        CHECK_EQUAL_FROM_OFFSET(read_buffer, offset, buffer1);
        offset += buffer1.size();
        CHECK_EQUAL_FROM_OFFSET(read_buffer, offset, buffer2);
        offset += buffer2.size();
        CHECK_EQUAL_FROM_OFFSET(read_buffer, offset, buffer3);
        offset += buffer3.size();
    }

    auto buffer2_refcounts = extract_refcounts(buffer2_address);
    ds->unlink(buffer2_refcounts);

    {
        shared_buffer<char> read_buffer(full_address.data_size());
        size_t t_read = 0;
        for (size_t i = 0; i < full_address.size(); ++i) {
            const auto p = full_address.get(i);
            auto read_size =
                ds->read(p.pointer, {read_buffer.data() + t_read, p.size});
            t_read += read_size;
        }

        BOOST_CHECK(t_read == full_address.data_size());
        shared_buffer<char> zero_buffer(buffer2.size());
        memset(zero_buffer.data(), 0, buffer2.size());

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

BOOST_AUTO_TEST_CASE(test_match_after_delete) {
    auto buffer1 = random_buffer(DEFAULT_PAGE_SIZE / 2);
    auto buffer2 = random_buffer(DEFAULT_PAGE_SIZE / 2);

    auto alloc1 = ds->allocate(buffer1.size());
    ds->write(alloc1, {buffer1.string_view()}, make_default_refcounts(alloc1));
    address buffer1_address;
    buffer1_address.emplace_back(alloc1.offset, buffer1.size());

    {
        shared_buffer<char> read_buffer(buffer1.size());
        size_t t_read = 0;
        for (size_t i = 0; i < buffer1_address.size(); ++i) {
            const auto p = buffer1_address.get(i);
            auto read_size =
                ds->read(p.pointer, {read_buffer.data() + t_read, p.size});
            t_read += read_size;
        }

        BOOST_CHECK(t_read == buffer1.size());
        BOOST_CHECK(std::memcmp(read_buffer.data(), buffer1.data(),
                                buffer1.size()) == 0);
    }

    auto buffer1_refcounts = extract_refcounts(buffer1_address);
    BOOST_CHECK(ds->unlink(buffer1_refcounts) == buffer1.size());

    {
        shared_buffer<char> zero_buffer(buffer1.size());
        memset(zero_buffer.data(), 0, zero_buffer.size());
        shared_buffer<char> read_buffer(buffer1.size());
        size_t t_read = 0;
        for (size_t i = 0; i < buffer1_address.size(); ++i) {
            const auto p = buffer1_address.get(i);
            auto read_size =
                ds->read(p.pointer, {read_buffer.data() + t_read, p.size});
            t_read += read_size;
        }

        BOOST_CHECK(t_read == buffer1.size());
        BOOST_CHECK(std::memcmp(read_buffer.data(), zero_buffer.data(),
                                zero_buffer.size()) == 0);
    }

    auto alloc2 = ds->allocate(buffer2.size());
    ds->write(alloc2, {buffer2.string_view()}, make_default_refcounts(alloc2));
    address buffer2_address;
    buffer2_address.emplace_back(alloc2.offset, buffer2.size());

    address combined_buffer_address;
    combined_buffer_address.append(buffer1_address);
    combined_buffer_address.append(buffer2_address);

    {
        shared_buffer<char> zero_buffer(buffer1.size());
        memset(zero_buffer.data(), 0, zero_buffer.size());
        shared_buffer<char> read_buffer(combined_buffer_address.data_size());
        size_t t_read = 0;
        for (size_t i = 0; i < combined_buffer_address.size(); ++i) {
            const auto p = combined_buffer_address.get(i);
            auto read_size =
                ds->read(p.pointer, {read_buffer.data() + t_read, p.size});
            t_read += read_size;
        }

        BOOST_CHECK(t_read == combined_buffer_address.data_size());
        BOOST_CHECK(std::memcmp(read_buffer.data(), zero_buffer.data(),
                                zero_buffer.size()) == 0);
        BOOST_CHECK(std::memcmp(read_buffer.data() + buffer1.size(),
                                buffer2.data(), buffer2.size()) == 0);
    }

    BOOST_CHECK(!ds->link(buffer1_refcounts).empty());
    auto buffer2_refcounts = extract_refcounts(buffer2_address);
    BOOST_CHECK(ds->unlink(buffer2_refcounts) == buffer2.size());

    {
        shared_buffer<char> zero_buffer(buffer1.size());
        memset(zero_buffer.data(), 0, zero_buffer.size());
        shared_buffer<char> read_buffer(combined_buffer_address.data_size());
        size_t t_read = 0;
        for (size_t i = 0; i < combined_buffer_address.size(); ++i) {
            const auto p = combined_buffer_address.get(i);
            auto read_size =
                ds->read(p.pointer, {read_buffer.data() + t_read, p.size});
            t_read += read_size;
        }

        BOOST_CHECK(t_read == combined_buffer_address.data_size());
        BOOST_CHECK(std::memcmp(read_buffer.data(), zero_buffer.data(),
                                zero_buffer.size()) == 0);
        BOOST_CHECK(std::memcmp(read_buffer.data() + buffer1.size(),
                                zero_buffer.data(), zero_buffer.size()) == 0);
    }

    BOOST_CHECK_THROW(ds->unlink(buffer1_refcounts), std::runtime_error);

    {
        shared_buffer<char> read_buffer(combined_buffer_address.data_size());
        shared_buffer<char> zero_buffer(combined_buffer_address.size());
        memset(zero_buffer.data(), 0, zero_buffer.size());
        size_t t_read = 0;
        for (size_t i = 0; i < combined_buffer_address.size(); ++i) {
            const auto p = combined_buffer_address.get(i);
            auto read_size =
                ds->read(p.pointer, {read_buffer.data() + t_read, p.size});
            t_read += read_size;
        }

        BOOST_CHECK(t_read == combined_buffer_address.data_size());
        BOOST_CHECK(std::memcmp(read_buffer.data(), zero_buffer.data(),
                                zero_buffer.size()) == 0);
    }
}

BOOST_AUTO_TEST_CASE(test_unlink_multi_file) {
    auto buffer1 = random_buffer(MAX_FILE_SIZE_BYTES - 1337);
    auto buffer2 = random_buffer(2 * DEFAULT_PAGE_SIZE);

    address full_address;
    auto alloc1 = ds->allocate(buffer1.size());
    auto alloc2 = ds->allocate(buffer2.size());
    ds->write(alloc1, {buffer1.string_view()}, make_default_refcounts(alloc1));
    ds->write(alloc2, {buffer2.string_view()}, make_default_refcounts(alloc2));
    address buffer1_address;
    buffer1_address.emplace_back(alloc1.offset, buffer1.size());
    address buffer2_address;
    buffer2_address.emplace_back(alloc2.offset, buffer2.size());
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
                ds->read(p.pointer, {read_buffer.data() + t_read, p.size});
            t_read += read_size;
        }

        BOOST_CHECK(t_read == full_address.data_size());
        BOOST_CHECK(std::memcmp(read_buffer.data(), buffer1.data(),
                                buffer1.size()) == 0);
        BOOST_CHECK(std::memcmp(read_buffer.data() + buffer1.size(),
                                buffer2.data(), buffer2.size()) == 0);
    }

    auto buffer2_refcounts = extract_refcounts(buffer2_address);
    ds->unlink(buffer2_refcounts);

    {
        shared_buffer<char> read_buffer(full_address.data_size());
        shared_buffer<char> zero_buffer(buffer2.size());
        memset(zero_buffer.data(), 0, zero_buffer.size());
        size_t t_read = 0;
        for (size_t i = 0; i < full_address.size(); ++i) {
            const auto p = full_address.get(i);
            auto read_size =
                ds->read(p.pointer, {read_buffer.data() + t_read, p.size});
            t_read += read_size;
        }

        BOOST_CHECK(t_read == full_address.data_size());

        BOOST_CHECK(std::memcmp(read_buffer.data(), buffer1.data(),
                                buffer1.size()) == 0);
        BOOST_CHECK(std::memcmp(read_buffer.data() + buffer1.size(),
                                zero_buffer.data(), zero_buffer.size()) == 0);
    }
}

BOOST_AUTO_TEST_CASE(repeated_write_delete) {
    auto buffer = random_buffer(MAX_FILE_SIZE_BYTES / 4);

    for (std::size_t i = 0; i < 100; i++) {
        auto alloc = ds->allocate(buffer.size());
        ds->write(alloc, {buffer.string_view()}, make_default_refcounts(alloc));
        address buffer_address;
        buffer_address.emplace_back(alloc.offset, buffer.size());
        auto buffer_refcounts = extract_refcounts(buffer_address);
        ds->unlink(buffer_refcounts);
    }

    auto alloc = ds->allocate(buffer.size());
    address buffer_address;
    buffer_address.emplace_back(alloc.offset, buffer.size());
    ds->write(alloc, {buffer.string_view()}, make_default_refcounts(alloc));

    shared_buffer<char> read_buffer(buffer_address.data_size());
    size_t t_read = 0;
    for (size_t i = 0; i < buffer_address.size(); ++i) {
        const auto p = buffer_address.get(i);
        auto read_size =
            ds->read(p.pointer, {read_buffer.data() + t_read, p.size});
        t_read += read_size;
    }

    BOOST_CHECK(t_read == buffer_address.data_size());
    BOOST_CHECK(std::memcmp(read_buffer.data(), buffer.data(), buffer.size()) ==
                0);
}

BOOST_AUTO_TEST_CASE(deletion_space_reclaim) {

    BOOST_CHECK_EQUAL(ds->get_used_space(), 0ull);
    BOOST_CHECK_EQUAL(ds->get_available_space(), MAX_DATA_STORE_SIZE_BYTES);

    auto data = random_buffer(MAX_DATA_STORE_SIZE_BYTES / 4);
    auto alloc = ds->allocate(data.size());
    ds->write(alloc, {data.string_view()}, make_default_refcounts(alloc));
    BOOST_CHECK_EQUAL(ds->get_used_space(), MAX_DATA_STORE_SIZE_BYTES / 4);
    BOOST_CHECK_EQUAL(ds->get_available_space(),
                      3 * (MAX_DATA_STORE_SIZE_BYTES / 4));
}

BOOST_AUTO_TEST_CASE(alignment) {
    auto alloc = ds->allocate(23, 1);
    BOOST_CHECK(alloc.offset % (1 * KIBI_BYTE) == 0);
    alloc = ds->allocate(42, 1);
    BOOST_CHECK(alloc.offset % (1 * KIBI_BYTE) != 0);
    alloc = ds->allocate(1 * KIBI_BYTE, 1 * KIBI_BYTE);
    BOOST_CHECK(alloc.offset % (1 * KIBI_BYTE) == 0);
}

BOOST_AUTO_TEST_SUITE_END()

} // end namespace uh::cluster
