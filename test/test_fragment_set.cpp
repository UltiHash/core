#define BOOST_TEST_MODULE "fragment set tests"

#include "common/utils/temp_directory.h"
#include "deduplicator/fragment_set.h"
#include "gdv_fixture.h"
#include <boost/test/unit_test.hpp>

// ------------- Tests Suites Follow --------------

namespace uh::cluster {

void insert_a(shared_buffer<char>& fragment_a, address& addr_a,
              fragment_set& frag_set) {
    auto result_a = frag_set.find(fragment_a.get_str_view());
    BOOST_CHECK(!result_a.low.has_value());
    BOOST_CHECK(!result_a.high.has_value());
    frag_set.insert(addr_a.first().pointer, fragment_a.get_str_view(),
                    result_a.hint);
}

void insert_a_again(shared_buffer<char>& fragment_a, address& addr_a,
                    fragment_set& frag_set) {
    auto result_a = frag_set.find(fragment_a.get_str_view());
    BOOST_CHECK(!result_a.low.has_value());
    BOOST_CHECK(result_a.high.has_value());
    auto prefix_a = shared_buffer<char>(sizeof(uint128_t));
    memcpy(prefix_a.data(), result_a.high->get().prefix().get_data(),
           sizeof(uint128_t));
    BOOST_CHECK(prefix_a.get_str_view() ==
                fragment_a.get_str_view().substr(0, sizeof(uint128_t)));
    BOOST_CHECK(result_a.high->get().pointer() == addr_a.first().pointer);
    BOOST_CHECK(result_a.high->get().size() == addr_a.first().size);
    frag_set.insert(addr_a.first().pointer, fragment_a.get_str_view(),
                    result_a.hint);
}

void insert_c(shared_buffer<char>& fragment_a, address& addr_a,
              shared_buffer<char>& fragment_c, address& addr_c,
              fragment_set& frag_set) {
    auto result_c = frag_set.find(fragment_c.get_str_view());
    frag_set.insert(addr_c.first().pointer, fragment_c.get_str_view(),
                    result_c.hint);
    BOOST_CHECK(result_c.low.has_value());
    auto prefix_a = shared_buffer<char>(sizeof(uint128_t));
    memcpy(prefix_a.data(), result_c.low->get().prefix().get_data(),
           sizeof(uint128_t));
    BOOST_CHECK(prefix_a.get_str_view() ==
                fragment_a.get_str_view().substr(0, sizeof(uint128_t)));
    BOOST_CHECK(result_c.low->get().pointer() == addr_a.first().pointer);
    BOOST_CHECK(result_c.low->get().size() == addr_a.first().size);
    BOOST_CHECK(!result_c.high.has_value());
}

void insert_b(shared_buffer<char>& fragment_a, address& addr_a,
              shared_buffer<char>& fragment_b, address& addr_b,
              shared_buffer<char>& fragment_c, address& addr_c,
              fragment_set& frag_set) {
    auto result_b = frag_set.find(fragment_b.get_str_view());
    frag_set.insert(addr_b.first().pointer, fragment_b.get_str_view(),
                    result_b.hint);
    BOOST_CHECK(result_b.low.has_value());
    auto prefix_b_low = shared_buffer<char>(sizeof(uint128_t));
    memcpy(prefix_b_low.data(), result_b.low->get().prefix().get_data(),
           sizeof(uint128_t));
    BOOST_CHECK(prefix_b_low.get_str_view() ==
                fragment_a.get_str_view().substr(0, sizeof(uint128_t)));
    BOOST_CHECK(result_b.low->get().pointer() == addr_a.first().pointer);
    BOOST_CHECK(result_b.low->get().size() == addr_a.first().size);
    BOOST_CHECK(result_b.high.has_value());
    auto prefix_b_high = shared_buffer<char>(sizeof(uint128_t));
    memcpy(prefix_b_high.data(), result_b.high->get().prefix().get_data(),
           sizeof(uint128_t));
    BOOST_CHECK(prefix_b_high.get_str_view() ==
                fragment_c.get_str_view().substr(0, sizeof(uint128_t)));
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
    auto addr_a = gdv->write(fragment_a.get_str_view());

    shared_buffer<char> fragment_b(4 * KIBI_BYTE);
    memset(fragment_b.data(), 'b', 4 * KIBI_BYTE);
    auto addr_b = gdv->write(fragment_b.get_str_view());

    shared_buffer<char> fragment_c(2 * KIBI_BYTE);
    memset(fragment_c.data(), 'c', 2 * KIBI_BYTE);
    auto addr_c = gdv->write(fragment_c.get_str_view());

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
    auto addr_a = gdv->write(fragment_a.get_str_view());
    gdv->add_l1(addr_a.first().pointer, fragment_a.get_str_view());

    shared_buffer<char> fragment_b(4 * KIBI_BYTE);
    memset(fragment_b.data(), 'b', 4 * KIBI_BYTE);
    auto addr_b = gdv->write(fragment_b.get_str_view());
    gdv->add_l1(addr_b.first().pointer, fragment_b.get_str_view());

    shared_buffer<char> fragment_c(2 * KIBI_BYTE);
    memset(fragment_c.data(), 'c', 2 * KIBI_BYTE);
    auto addr_c = gdv->write(fragment_c.get_str_view());
    gdv->add_l1(addr_c.first().pointer, fragment_c.get_str_view());

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
    auto addr_a = gdv->write(fragment_a.get_str_view());

    shared_buffer<char> fragment_b(8 * KIBI_BYTE);
    memset(fragment_b.data(), 'a', 2 * KIBI_BYTE);
    memset(fragment_b.data() + 2 * KIBI_BYTE, 'b', 2 * KIBI_BYTE);
    memset(fragment_b.data() + 4 * KIBI_BYTE, 'a', 2 * KIBI_BYTE);
    memset(fragment_b.data() + 4 * KIBI_BYTE, 'b', 2 * KIBI_BYTE);

    auto addr_b = gdv->write(fragment_b.get_str_view());

    shared_buffer<char> fragment_c(8 * KIBI_BYTE);
    memset(fragment_c.data(), 'a', 2 * KIBI_BYTE);
    memset(fragment_c.data() + 2 * KIBI_BYTE, 'b', 2 * KIBI_BYTE);
    memset(fragment_c.data() + 4 * KIBI_BYTE, 'a', 2 * KIBI_BYTE);
    memset(fragment_c.data() + 4 * KIBI_BYTE, 'c', 2 * KIBI_BYTE);
    auto addr_c = gdv->write(fragment_c.get_str_view());

    // This will create fragment_elements which ONLY contain the prefix
    fragment_set_element frag_element_a(fragment_a.get_str_view(),
                                        addr_a.first().pointer, *gdv);
    fragment_set_element frag_element_b(fragment_b.get_str_view(),
                                        addr_b.first().pointer, *gdv);
    fragment_set_element frag_element_c(fragment_c.get_str_view(),
                                        addr_c.first().pointer, *gdv);

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
        .size = 13,
        .prefix = {0x707265666978}};

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
