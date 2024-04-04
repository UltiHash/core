#define BOOST_TEST_MODULE "global data view tests"

#include "gdv_fixture.h"
#include <boost/test/unit_test.hpp>

// ------------- Tests Suites Follow --------------

namespace uh::cluster {

static void fill_random(char* buf, size_t size) {
    for (size_t i = 0; i < size; ++i) {
        buf[i] = rand() & 0xff;
    }
}

BOOST_FIXTURE_TEST_CASE(invalid_read_fragment, global_data_view_fixture) {
    auto gdv = get_global_data_view();
    BOOST_CHECK_THROW(
        gdv->read_fragment(std::numeric_limits<uint64_t>::max(), 8 * KIBI_BYTE),
        std::runtime_error);
}

BOOST_FIXTURE_TEST_CASE(valid_write_read_fragment, global_data_view_fixture) {
    auto gdv = get_global_data_view();
    auto input_buffer = unique_buffer<char>(8 * KIBI_BYTE);
    fill_random(input_buffer.data(), input_buffer.size());
    auto addr = gdv->write(input_buffer.get_str_view());
    BOOST_CHECK(input_buffer.size() == addr.data_size());
    BOOST_CHECK(addr.pointers.size() == 2);
    BOOST_CHECK(addr.sizes.size() == 1);
    gdv->sync(addr);

    auto frag = addr.first();
    auto result_buffer = gdv->read_fragment(frag.pointer, frag.size);
    BOOST_CHECK(input_buffer.get_str_view() == result_buffer.get_str_view());
}

BOOST_FIXTURE_TEST_CASE(invalid_read_address, global_data_view_fixture) {
    auto gdv = get_global_data_view();
    address addr;
    addr.push_fragment({23, 42});
    auto result_buffer = unique_buffer<char>(addr.data_size());

    BOOST_CHECK_THROW(gdv->read_address(result_buffer.data(), addr),
                      std::runtime_error);
}

BOOST_FIXTURE_TEST_CASE(valid_write_read_address, global_data_view_fixture) {
    auto gdv = get_global_data_view();
    auto input_buffer = unique_buffer<char>(64 * KIBI_BYTE);
    fill_random(input_buffer.data(), input_buffer.size());

    address addr;
    addr.append_address(gdv->write(
        input_buffer.get_str_view().substr(0 * KIBI_BYTE, 8 * KIBI_BYTE)));
    addr.append_address(gdv->write(
        input_buffer.get_str_view().substr(8 * KIBI_BYTE, 8 * KIBI_BYTE)));
    addr.append_address(gdv->write(
        input_buffer.get_str_view().substr(16 * KIBI_BYTE, 8 * KIBI_BYTE)));
    addr.append_address(gdv->write(
        input_buffer.get_str_view().substr(24 * KIBI_BYTE, 8 * KIBI_BYTE)));
    addr.append_address(gdv->write(
        input_buffer.get_str_view().substr(32 * KIBI_BYTE, 8 * KIBI_BYTE)));
    addr.append_address(gdv->write(
        input_buffer.get_str_view().substr(40 * KIBI_BYTE, 8 * KIBI_BYTE)));
    addr.append_address(gdv->write(
        input_buffer.get_str_view().substr(48 * KIBI_BYTE, 8 * KIBI_BYTE)));
    addr.append_address(gdv->write(
        input_buffer.get_str_view().substr(56 * KIBI_BYTE, 8 * KIBI_BYTE)));

    BOOST_CHECK(input_buffer.size() == addr.data_size());
    gdv->sync(addr);

    auto result_buffer = unique_buffer<char>(addr.data_size());
    gdv->read_address(result_buffer.data(), addr);
    BOOST_CHECK(input_buffer.get_str_view() == result_buffer.get_str_view());
}

BOOST_FIXTURE_TEST_CASE(invalid_cached_sample, global_data_view_fixture) {
    auto gdv = get_global_data_view();
    auto sample = gdv->cached_sample(std::numeric_limits<uint64_t>::max());
    BOOST_CHECK(sample.data() == nullptr);
}

BOOST_FIXTURE_TEST_CASE(valid_cached_sample, global_data_view_fixture) {
    auto gdv = get_global_data_view();
    auto input_buffer = unique_buffer<char>(8 * KIBI_BYTE);
    fill_random(input_buffer.data(), input_buffer.size());
    auto addr = gdv->write(input_buffer.get_str_view());
    BOOST_CHECK(input_buffer.size() == addr.data_size());
    BOOST_CHECK(addr.pointers.size() == 2);
    BOOST_CHECK(addr.sizes.size() == 1);
    gdv->sync(addr);

    gdv->add_l1(addr.first().pointer, input_buffer.get_str_view());

    auto frag = addr.first();
    auto short_sample = gdv->cached_sample(frag.pointer);
    BOOST_CHECK(short_sample.size() == gdv->l1_cache_sample_size());
    BOOST_CHECK(
        input_buffer.get_str_view().substr(0, gdv->l1_cache_sample_size()) ==
        short_sample.get_str_view());
}

} // namespace uh::cluster
