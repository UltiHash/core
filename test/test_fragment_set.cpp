#define BOOST_TEST_MODULE "fragment set tests"

#include "common/utils/temp_directory.h"
#include "deduplicator/dedupe_set/fragment_set.h"
#include "gdv_fixture.h"
#include <boost/test/unit_test.hpp>

// ------------- Tests Suites Follow --------------

namespace uh::cluster {

void insert_a(shared_buffer<char>& fragment_a, address& addr_a,
              fragment_set& frag_set) {
    auto result_a = frag_set.find(fragment_a.string_view());
    BOOST_CHECK(!result_a.low.has_value());
    BOOST_CHECK(!result_a.high.has_value());
    frag_set.insert(addr_a.first().pointer,
                    fragment_a.string_view().substr(0, addr_a.first().size),
                    result_a.hint);
}

void insert_a_again(shared_buffer<char>& fragment_a, address& addr_a,
                    fragment_set& frag_set) {
    auto result_a = frag_set.find(fragment_a.string_view());
    BOOST_CHECK(!result_a.low.has_value());
    BOOST_CHECK(result_a.high.has_value());
    auto prefix_a = shared_buffer<char>(PREFIX_SIZE);
    memcpy(prefix_a.data(), result_a.high->get().prefix().data(),
           result_a.high->get().prefix().size());
    BOOST_CHECK(prefix_a.string_view() ==
                fragment_a.string_view().substr(0, PREFIX_SIZE));
    BOOST_CHECK(result_a.high->get().pointer() == addr_a.first().pointer);
    BOOST_CHECK(result_a.high->get().size() == addr_a.first().size);
    frag_set.insert(addr_a.first().pointer,
                    fragment_a.string_view().substr(0, addr_a.first().size),
                    result_a.hint);
}

void insert_c(shared_buffer<char>& fragment_a, address& addr_a,
              shared_buffer<char>& fragment_c, address& addr_c,
              fragment_set& frag_set) {
    auto result_c = frag_set.find(fragment_c.string_view());
    frag_set.insert(addr_c.first().pointer,
                    fragment_c.string_view().substr(0, addr_c.first().size),
                    result_c.hint);
    BOOST_CHECK(result_c.low.has_value());
    auto prefix_a = shared_buffer<char>(PREFIX_SIZE);
    memcpy(prefix_a.data(), result_c.low->get().prefix().data(),
           result_c.low->get().prefix().size());
    BOOST_CHECK(prefix_a.string_view() ==
                fragment_a.string_view().substr(0, PREFIX_SIZE));
    BOOST_CHECK(result_c.low->get().pointer() == addr_a.first().pointer);
    BOOST_CHECK(result_c.low->get().size() == addr_a.first().size);
    BOOST_CHECK(!result_c.high.has_value());
}

void insert_b(shared_buffer<char>& fragment_a, address& addr_a,
              shared_buffer<char>& fragment_b, address& addr_b,
              shared_buffer<char>& fragment_c, address& addr_c,
              fragment_set& frag_set) {
    auto result_b = frag_set.find(fragment_b.string_view());
    frag_set.insert(addr_b.first().pointer,
                    fragment_b.string_view().substr(0, addr_b.first().size),
                    result_b.hint);
    BOOST_CHECK(result_b.low.has_value());
    auto prefix_b_low = shared_buffer<char>(PREFIX_SIZE);
    memcpy(prefix_b_low.data(), result_b.low->get().prefix().data(),
           result_b.low->get().prefix().size());
    BOOST_CHECK(prefix_b_low.string_view() ==
                fragment_a.string_view().substr(0, PREFIX_SIZE));
    BOOST_CHECK(result_b.low->get().pointer() == addr_a.first().pointer);
    BOOST_CHECK(result_b.low->get().size() == addr_a.first().size);
    BOOST_CHECK(result_b.high.has_value());
    auto prefix_b_high = shared_buffer<char>(PREFIX_SIZE);
    memcpy(prefix_b_high.data(), result_b.high->get().prefix().data(),
           result_b.high->get().prefix().size());
    BOOST_CHECK(prefix_b_high.string_view() ==
                fragment_c.string_view().substr(0, PREFIX_SIZE));
    BOOST_CHECK(result_b.high->get().pointer() == addr_c.first().pointer);
    BOOST_CHECK(result_b.high->get().size() == addr_c.first().size);
}

BOOST_FIXTURE_TEST_CASE(insert_find_basic, global_data_view_fixture) {
    temp_directory tmp_dir;
    std::filesystem::path frag_set_log_path = tmp_dir.path() / "logfile";
    auto gdv = get_global_data_view();
    fragment_set frag_set(frag_set_log_path, *gdv);

    shared_buffer<char> fragment_a(8 * KIBI_BYTE);
    memset(fragment_a.data(), 'a', 8 * KIBI_BYTE);
    auto addr_a = gdv->write(fragment_a.string_view());

    shared_buffer<char> fragment_b(4 * KIBI_BYTE);
    memset(fragment_b.data(), 'b', 4 * KIBI_BYTE);
    auto addr_b = gdv->write(fragment_b.string_view());

    shared_buffer<char> fragment_c(2 * KIBI_BYTE);
    memset(fragment_c.data(), 'c', 2 * KIBI_BYTE);
    auto addr_c = gdv->write(fragment_c.string_view());

    insert_a(fragment_a, addr_a, frag_set);
    insert_a_again(fragment_a, addr_a, frag_set);
    insert_c(fragment_a, addr_a, fragment_c, addr_c, frag_set);
    insert_b(fragment_a, addr_a, fragment_b, addr_b, fragment_c, addr_c,
             frag_set);
}

BOOST_FIXTURE_TEST_CASE(insert_find_rebuild, global_data_view_fixture) {
    temp_directory tmp_dir;
    std::filesystem::path frag_set_log_path = tmp_dir.path() / "logfile";
    auto gdv = get_global_data_view();

    shared_buffer<char> fragment_a(8 * KIBI_BYTE);
    memset(fragment_a.data(), 'a', 8 * KIBI_BYTE);
    auto addr_a = gdv->write(fragment_a.string_view());

    shared_buffer<char> fragment_b(4 * KIBI_BYTE);
    memset(fragment_b.data(), 'b', 4 * KIBI_BYTE);
    auto addr_b = gdv->write(fragment_b.string_view());

    shared_buffer<char> fragment_c(2 * KIBI_BYTE);
    memset(fragment_c.data(), 'c', 2 * KIBI_BYTE);
    auto addr_c = gdv->write(fragment_c.string_view());

    {
        fragment_set frag_set(frag_set_log_path, *gdv);
        insert_a(fragment_a, addr_a, frag_set);
        insert_a_again(fragment_a, addr_a, frag_set);
    }

    // destruct frag set and reconstruct it again from frag_set_log_path
    {
        fragment_set frag_set(frag_set_log_path, *gdv);
        insert_c(fragment_a, addr_a, fragment_c, addr_c, frag_set);
    }

    // destruct frag set and reconstruct it again from frag_set_log_path
    {
        fragment_set frag_set(frag_set_log_path, *gdv);
        insert_b(fragment_a, addr_a, fragment_b, addr_b, fragment_c, addr_c,
                 frag_set);
    }
}

BOOST_FIXTURE_TEST_CASE(less_operator, global_data_view_fixture) {
    temp_directory tmp_dir;
    std::filesystem::path frag_set_log_path = tmp_dir.path() / "logfile";
    auto gdv = get_global_data_view();
    fragment_set frag_set(frag_set_log_path, *gdv);

    shared_buffer<char> fragment_a(8 * KIBI_BYTE);
    memset(fragment_a.data(), 'a', 8 * KIBI_BYTE);
    auto addr_a = gdv->write(fragment_a.string_view());

    shared_buffer<char> fragment_b(8 * KIBI_BYTE);
    memset(fragment_b.data(), 'a', 2 * KIBI_BYTE);
    memset(fragment_b.data() + 2 * KIBI_BYTE, 'b', 2 * KIBI_BYTE);
    memset(fragment_b.data() + 4 * KIBI_BYTE, 'a', 2 * KIBI_BYTE);
    memset(fragment_b.data() + 4 * KIBI_BYTE, 'b', 2 * KIBI_BYTE);

    auto addr_b = gdv->write(fragment_b.string_view());

    shared_buffer<char> fragment_c(8 * KIBI_BYTE);
    memset(fragment_c.data(), 'a', 2 * KIBI_BYTE);
    memset(fragment_c.data() + 2 * KIBI_BYTE, 'c', 2 * KIBI_BYTE);
    memset(fragment_c.data() + 4 * KIBI_BYTE, 'a', 2 * KIBI_BYTE);
    memset(fragment_c.data() + 4 * KIBI_BYTE, 'c', 2 * KIBI_BYTE);
    auto addr_c = gdv->write(fragment_c.string_view());

    // This will create fragment_elements which ONLY contain the prefix
    auto prefix_a = fragment_a.string_view().substr(
        0, std::min(PREFIX_SIZE, fragment_a.size()));
    auto prefix_b = fragment_b.string_view().substr(
        0, std::min(PREFIX_SIZE, fragment_b.size()));
    auto prefix_c = fragment_c.string_view().substr(
        0, std::min(PREFIX_SIZE, fragment_c.size()));

    fragment_set_element frag_element_a(
        fragment_a.string_view().substr(0, addr_a.first().size),
        addr_a.first().pointer, std::string(prefix_a), *gdv);
    fragment_set_element frag_element_b(
        fragment_b.string_view().substr(0, addr_b.first().size),
        addr_b.first().pointer, std::string(prefix_b), *gdv);
    fragment_set_element frag_element_c(
        fragment_c.string_view().substr(0, addr_c.first().size),
        addr_c.first().pointer, std::string(prefix_c), *gdv);

    // Since all fragments have identical prefix, calling operator< will be
    // forced to consult gdv to get full body
    BOOST_CHECK(frag_element_a < frag_element_b);
    BOOST_CHECK(frag_element_a < frag_element_c);
    BOOST_CHECK(frag_element_b < frag_element_c);
}

BOOST_FIXTURE_TEST_CASE(insert_performance, global_data_view_fixture) {
    temp_directory tmp_dir;
    std::filesystem::path frag_set_log_path = tmp_dir.path() / "logfile";
    auto log = fragment_set_log(frag_set_log_path);

    fragment_set_log::log_entry entry = {
        .op = INSERT,
        .pointer = {0x6465647570, 0x6c69636174696f6e},
        .size = 13};

    const auto start = std::chrono::steady_clock::now();
    for (std::size_t i = 0; i < 10000000; i++) {
        log.append(entry);
    }
    log.flush();
    const auto duration = std::chrono::steady_clock::now() - start;
    auto millis =
        std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    LOG_INFO() << "Inserting 10,000,000 log entries took " << millis << "ms.";
}

} // namespace uh::cluster
