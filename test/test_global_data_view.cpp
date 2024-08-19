#define BOOST_TEST_MODULE "global data view tests"

#include "common/utils/strings.h"
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
    context ctx;
    BOOST_CHECK_THROW(gdv->read_fragment(ctx,
                                         std::numeric_limits<uint64_t>::max(),
                                         8 * KIBI_BYTE),
                      std::runtime_error);
}

BOOST_FIXTURE_TEST_CASE(valid_write_read_fragment, global_data_view_fixture) {
    auto gdv = get_global_data_view();
    context ctx;

    auto input_buffer = unique_buffer<char>(8 * KIBI_BYTE);
    fill_random(input_buffer.data(), input_buffer.size());
    auto addr =
        boost::asio::co_spawn(gdv->get_executor(),
                              gdv->write(ctx, input_buffer.string_view()),
                              boost::asio::use_future)
            .get();
    BOOST_CHECK(input_buffer.size() == addr.data_size());
    BOOST_TEST(addr.pointers.size() == 2ul);
    BOOST_TEST(addr.sizes.size() == 1ul);
    boost::asio::co_spawn(gdv->get_executor(), gdv->sync(ctx, addr),
                          boost::asio::use_future)
        .get();

    unique_buffer<char> result_buffer(addr.data_size());
    boost::asio::co_spawn(gdv->get_executor(),
                          gdv->read_address(ctx, result_buffer.data(), addr),
                          boost::asio::use_future)
        .get();
    BOOST_CHECK(input_buffer.string_view() == result_buffer.string_view());
}

BOOST_FIXTURE_TEST_CASE(invalid_read_address, global_data_view_fixture) {
    auto gdv = get_global_data_view();
    context ctx;

    address addr;
    addr.push({23, 42});
    auto result_buffer = unique_buffer<char>(addr.data_size());

    BOOST_CHECK_THROW(boost::asio::co_spawn(
                          gdv->get_executor(),
                          gdv->read_address(ctx, result_buffer.data(), addr),
                          boost::asio::use_future)
                          .get(),
                      std::runtime_error);
}

BOOST_FIXTURE_TEST_CASE(valid_write_read_address, global_data_view_fixture) {
    auto gdv = get_global_data_view();
    context ctx;

    auto input_buffer = unique_buffer<char>(64 * KIBI_BYTE);
    fill_random(input_buffer.data(), input_buffer.size());

    address addr;
    addr.append(boost::asio::co_spawn(
                    gdv->get_executor(),
                    gdv->write(ctx, input_buffer.string_view().substr(
                                        0 * KIBI_BYTE, 8 * KIBI_BYTE)),
                    boost::asio::use_future)
                    .get());
    addr.append(boost::asio::co_spawn(
                    gdv->get_executor(),
                    gdv->write(ctx, input_buffer.string_view().substr(
                                        8 * KIBI_BYTE, 8 * KIBI_BYTE)),
                    boost::asio::use_future)
                    .get());
    addr.append(boost::asio::co_spawn(
                    gdv->get_executor(),
                    gdv->write(ctx, input_buffer.string_view().substr(
                                        16 * KIBI_BYTE, 8 * KIBI_BYTE)),
                    boost::asio::use_future)
                    .get());
    addr.append(boost::asio::co_spawn(
                    gdv->get_executor(),
                    gdv->write(ctx, input_buffer.string_view().substr(
                                        24 * KIBI_BYTE, 8 * KIBI_BYTE)),
                    boost::asio::use_future)
                    .get());
    addr.append(boost::asio::co_spawn(
                    gdv->get_executor(),
                    gdv->write(ctx, input_buffer.string_view().substr(
                                        32 * KIBI_BYTE, 8 * KIBI_BYTE)),
                    boost::asio::use_future)
                    .get());
    addr.append(boost::asio::co_spawn(
                    gdv->get_executor(),
                    gdv->write(ctx, input_buffer.string_view().substr(
                                        40 * KIBI_BYTE, 8 * KIBI_BYTE)),
                    boost::asio::use_future)
                    .get());
    addr.append(boost::asio::co_spawn(
                    gdv->get_executor(),
                    gdv->write(ctx, input_buffer.string_view().substr(
                                        48 * KIBI_BYTE, 8 * KIBI_BYTE)),
                    boost::asio::use_future)
                    .get());
    addr.append(boost::asio::co_spawn(
                    gdv->get_executor(),
                    gdv->write(ctx, input_buffer.string_view().substr(
                                        56 * KIBI_BYTE, 8 * KIBI_BYTE)),
                    boost::asio::use_future)
                    .get());

    BOOST_CHECK(input_buffer.size() == addr.data_size());
    boost::asio::co_spawn(gdv->get_executor(), gdv->sync(ctx, addr),
                          boost::asio::use_future)
        .get();

    auto result_buffer = unique_buffer<char>(addr.data_size());
    boost::asio::co_spawn(gdv->get_executor(),
                          gdv->read_address(ctx, result_buffer.data(), addr),
                          boost::asio::use_future)
        .get();
    BOOST_CHECK(input_buffer.string_view() == result_buffer.string_view());
}

} // namespace uh::cluster
