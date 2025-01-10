#define BOOST_TEST_MODULE "fragment set tests"

#include "common/utils/temp_directory.h"
#include "deduplicator/dedupe_set/fragment_set.h"
#include "deduplicator/dedupe_set/fragment_set_log.h"
#include "gdv_fixture.h"
#include <boost/test/unit_test.hpp>

// ------------- Tests Suites Follow --------------

namespace uh::cluster {

struct fragment_set_fixture : public global_data_view_fixture {
    temp_directory tmp_dir;
    context ctx;
    std::shared_ptr<global_data_view> gdv;
    std::shared_ptr<fragment_set> frag_set;

    fragment_set_fixture() {}

    void setup() {
        global_data_view_fixture::setup();
        gdv = get_global_data_view();
        frag_set = std::make_shared<fragment_set>(tmp_dir.path() / "logfile",
                                                  1000, *gdv);
    }

    std::pair<shared_buffer<char>, address> create_fragment(char fill_char,
                                                            std::size_t size) {
        shared_buffer<char> fragment(size);
        memset(fragment.data(), fill_char, size);
        auto addr =
            boost::asio::co_spawn(gdv->get_executor(),
                                  gdv->write(ctx, fragment.string_view(), {0}),
                                  boost::asio::use_future)
                .get();
        return {std::move(fragment), addr};
    }
    shared_buffer<char> read_prefix(const std::string& source,
                                    std::size_t size) {
        shared_buffer<char> prefix(size);
        memcpy(prefix.data(), source.data(), size);
        return prefix;
    }
};

BOOST_FIXTURE_TEST_SUITE(a_fragment_set, fragment_set_fixture)

BOOST_AUTO_TEST_CASE(finds_no_fragment_which_wasnt_inserted_yet) {
    auto [fragment_a, addr_a] = create_fragment('a', 8 * KIBI_BYTE);

    auto result_a = frag_set->find(fragment_a.string_view());

    BOOST_TEST(!result_a.low.has_value());
    BOOST_TEST(!result_a.high.has_value());
}

BOOST_AUTO_TEST_CASE(inserts_new_value) {
    auto [fragment_a, addr_a] = create_fragment('a', 8 * KIBI_BYTE);

    frag_set->insert(addr_a.get(0).pointer,
                     fragment_a.string_view().substr(0, addr_a.get(0).size),
                     false, frag_set->find(fragment_a.string_view()).hint);
    auto result_a = frag_set->find(fragment_a.string_view());

    BOOST_TEST(!result_a.low.has_value());
    BOOST_TEST(result_a.high.has_value());
    BOOST_TEST(result_a.high->second.size() == PREFIX_SIZE);
    BOOST_TEST(result_a.high->second ==
               fragment_a.string_view().substr(0, PREFIX_SIZE));
    BOOST_TEST(result_a.high->first.pointer == addr_a.get(0).pointer);
    BOOST_TEST(result_a.high->first.size == addr_a.get(0).size);
}

BOOST_AUTO_TEST_CASE(handles_duplicate_insertion_correctly) {
    auto [fragment_a, addr_a] = create_fragment('a', 8 * KIBI_BYTE);

    for (int i = 0; i < 2; ++i) {
        frag_set->insert(addr_a.get(0).pointer,
                         fragment_a.string_view().substr(0, addr_a.get(0).size),
                         false, frag_set->find(fragment_a.string_view()).hint);
    }
    auto result_a = frag_set->find(fragment_a.string_view());

    BOOST_TEST(!result_a.low.has_value());
    BOOST_TEST(result_a.high.has_value());
    BOOST_TEST(result_a.high->second == fragment_a.string_view().substr(
                                            0, result_a.high->second.size()));
    BOOST_TEST(result_a.high->first.pointer == addr_a.get(0).pointer);
    BOOST_TEST(result_a.high->first.size == addr_a.get(0).size);
}

BOOST_AUTO_TEST_CASE(finds_low_value_after_inserting_smaller_key) {
    auto [fragment_a, addr_a] = create_fragment('a', 8 * KIBI_BYTE);
    frag_set->insert(addr_a.get(0).pointer,
                     fragment_a.string_view().substr(0, addr_a.get(0).size),
                     false, frag_set->find(fragment_a.string_view()).hint);

    auto [fragment_c, addr_c] = create_fragment('c', 2 * KIBI_BYTE);
    auto result_c = frag_set->find(fragment_c.string_view());

    BOOST_TEST(result_c.low.has_value());
    BOOST_TEST(result_c.low->second ==
               fragment_a.string_view().substr(0, result_c.low->second.size()));
    BOOST_TEST(result_c.low->first.pointer == addr_a.get(0).pointer);
    BOOST_TEST(result_c.low->first.size == addr_a.get(0).size);
    BOOST_TEST(!result_c.high.has_value());
}

BOOST_AUTO_TEST_CASE(finds_both_low_and_high_values_correctly) {
    auto [fragment_a, addr_a] = create_fragment('a', 8 * KIBI_BYTE);
    auto [fragment_c, addr_c] = create_fragment('c', 2 * KIBI_BYTE);
    frag_set->insert(addr_a.get(0).pointer,
                     fragment_a.string_view().substr(0, addr_a.get(0).size),
                     false, frag_set->find(fragment_a.string_view()).hint);
    frag_set->insert(addr_c.get(0).pointer,
                     fragment_c.string_view().substr(0, addr_c.get(0).size),
                     false, frag_set->find(fragment_c.string_view()).hint);

    auto [fragment_b, addr_b] = create_fragment('b', 4 * KIBI_BYTE);
    auto result_b = frag_set->find(fragment_b.string_view());

    BOOST_TEST(result_b.low.has_value());
    BOOST_TEST(result_b.low->second ==
               fragment_a.string_view().substr(0, result_b.low->second.size()));
    BOOST_TEST(result_b.low->first.pointer == addr_a.get(0).pointer);
    BOOST_TEST(result_b.low->first.size == addr_a.get(0).size);

    BOOST_TEST(result_b.high.has_value());
    BOOST_TEST(result_b.high->second == fragment_c.string_view().substr(
                                            0, result_b.high->second.size()));
    BOOST_TEST(result_b.high->first.pointer == addr_c.get(0).pointer);
    BOOST_TEST(result_b.high->first.size == addr_c.get(0).size);
}

BOOST_AUTO_TEST_CASE(inserts_sandwitched_fragment) {
    auto [fragment_a, addr_a] = create_fragment('a', 8 * KIBI_BYTE);
    auto [fragment_c, addr_c] = create_fragment('c', 2 * KIBI_BYTE);
    frag_set->insert(addr_a.get(0).pointer,
                     fragment_a.string_view().substr(0, addr_a.get(0).size),
                     false, frag_set->find(fragment_a.string_view()).hint);
    frag_set->insert(addr_c.get(0).pointer,
                     fragment_c.string_view().substr(0, addr_c.get(0).size),
                     false, frag_set->find(fragment_c.string_view()).hint);

    auto [fragment_b, addr_b] = create_fragment('b', 4 * KIBI_BYTE);
    BOOST_REQUIRE_NO_THROW({
        frag_set->insert(addr_b.get(0).pointer,
                         fragment_b.string_view().substr(0, addr_b.get(0).size),
                         false, frag_set->find(fragment_b.string_view()).hint);
    });
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_FIXTURE_TEST_CASE(less_operator, global_data_view_fixture) {
    temp_directory tmp_dir;
    context ctx;

    std::filesystem::path frag_set_log_path = tmp_dir.path() / "logfile";
    auto gdv = get_global_data_view();
    fragment_set frag_set(frag_set_log_path, 1000, *gdv);

    shared_buffer<char> fragment_a(8 * KIBI_BYTE);
    memset(fragment_a.data(), 'a', 8 * KIBI_BYTE);
    auto addr_a =
        boost::asio::co_spawn(gdv->get_executor(),
                              gdv->write(ctx, fragment_a.string_view(), {0}),
                              boost::asio::use_future)
            .get();

    shared_buffer<char> fragment_b(8 * KIBI_BYTE);
    memset(fragment_b.data(), 'a', 2 * KIBI_BYTE);
    memset(fragment_b.data() + 2 * KIBI_BYTE, 'b', 2 * KIBI_BYTE);
    memset(fragment_b.data() + 4 * KIBI_BYTE, 'a', 2 * KIBI_BYTE);
    memset(fragment_b.data() + 4 * KIBI_BYTE, 'b', 2 * KIBI_BYTE);

    auto addr_b =
        boost::asio::co_spawn(gdv->get_executor(),
                              gdv->write(ctx, fragment_b.string_view(), {0}),
                              boost::asio::use_future)
            .get();

    shared_buffer<char> fragment_c(8 * KIBI_BYTE);
    memset(fragment_c.data(), 'a', 2 * KIBI_BYTE);
    memset(fragment_c.data() + 2 * KIBI_BYTE, 'c', 2 * KIBI_BYTE);
    memset(fragment_c.data() + 4 * KIBI_BYTE, 'a', 2 * KIBI_BYTE);
    memset(fragment_c.data() + 4 * KIBI_BYTE, 'c', 2 * KIBI_BYTE);
    auto addr_c =
        boost::asio::co_spawn(gdv->get_executor(),
                              gdv->write(ctx, fragment_c.string_view(), {0}),
                              boost::asio::use_future)
            .get();

    // This will create fragment_elements which ONLY contain the prefix
    auto prefix_a = fragment_a.string_view().substr(
        0, std::min(PREFIX_SIZE, fragment_a.size()));
    auto prefix_b = fragment_b.string_view().substr(
        0, std::min(PREFIX_SIZE, fragment_b.size()));
    auto prefix_c = fragment_c.string_view().substr(
        0, std::min(PREFIX_SIZE, fragment_c.size()));

    fragment_set_element frag_element_a(
        fragment_a.string_view().substr(0, addr_a.get(0).size),
        addr_a.get(0).pointer, std::string(prefix_a), *gdv);
    fragment_set_element frag_element_b(
        fragment_b.string_view().substr(0, addr_b.get(0).size),
        addr_b.get(0).pointer, std::string(prefix_b), *gdv);
    fragment_set_element frag_element_c(
        fragment_c.string_view().substr(0, addr_c.get(0).size),
        addr_c.get(0).pointer, std::string(prefix_c), *gdv);

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

    fragment_set_log::log_entry entry(INSERT,
                                      {0x6465647570, 0x6c69636174696f6e}, 13);

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
