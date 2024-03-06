#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "global data view tests"
#endif

#include "gdv_fixture.h"
#include <boost/test/unit_test.hpp>

// ------------- Tests Suites Follow --------------

namespace uh::cluster {

static void fill_random(char* buf, size_t size) {
    for (size_t i = 0; i < size; ++i) {
        buf[i] = rand() & 0xff;
    }
}

BOOST_FIXTURE_TEST_CASE(fixture_test1, global_data_view_fixture) {
    auto gdv = get_global_data_view();
    auto input_buffer = unique_buffer<char>(256);
    fill_random(input_buffer.data(), input_buffer.size());
    auto addr = gdv->write(input_buffer.get_str_view());
    BOOST_CHECK(input_buffer.size() == addr.data_size());
    gdv->sync(addr);

    auto result_buffer = unique_buffer<char>(addr.data_size());
    gdv->read_address(result_buffer.data(), addr);
    BOOST_CHECK(input_buffer.get_str_view() == result_buffer.get_str_view());
}

BOOST_FIXTURE_TEST_CASE(fixture_test2, global_data_view_fixture) {
    auto gdv = get_global_data_view();
    auto input_buffer = unique_buffer<char>(256);
    fill_random(input_buffer.data(), input_buffer.size());
    auto addr = gdv->write(input_buffer.get_str_view());
    BOOST_CHECK(input_buffer.size() == addr.data_size());
    gdv->sync(addr);

    auto result_buffer = unique_buffer<char>(addr.data_size());
    gdv->read_address(result_buffer.data(), addr);
    BOOST_CHECK(input_buffer.get_str_view() == result_buffer.get_str_view());
}

BOOST_FIXTURE_TEST_CASE(fixture_test3, global_data_view_fixture) {
    auto gdv = get_global_data_view();
    auto input_buffer = unique_buffer<char>(256);
    fill_random(input_buffer.data(), input_buffer.size());
    auto addr = gdv->write(input_buffer.get_str_view());
    BOOST_CHECK(input_buffer.size() == addr.data_size());
    gdv->sync(addr);

    auto result_buffer = unique_buffer<char>(addr.data_size());
    gdv->read_address(result_buffer.data(), addr);
    BOOST_CHECK(input_buffer.get_str_view() == result_buffer.get_str_view());
}
} // namespace uh::cluster
