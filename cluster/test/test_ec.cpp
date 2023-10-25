//
// Created by masi on 10/24/23.
//

#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "EC tests"
#endif

#include <boost/test/unit_test.hpp>
#include <boost/asio/awaitable.hpp>

#include "global_data_view_fixture.h"



// ------------- Tests Suites Follow --------------

namespace uh::cluster {


void fill_random(char* buf, size_t size) {
    for (int i = 0; i < size; ++i) {
        buf[i] = rand()&0xff;
    }
}

BOOST_FIXTURE_TEST_CASE (basic_write_read_test, global_data_view_fixture)
{
    setup(4, 1, 0);

    constexpr auto data_size = 3ul*1024ul;
    char data [data_size];

    fill_random(data, data_size);

    std::promise <address> alloc_promise;
    const auto data_str = std::string_view (data, data_size);
    auto write_data = [&] () -> coro <message_type> {
        auto alloc = co_await get_dedupe_node(0).get_global_data_view().allocate(data_size);
        co_await get_dedupe_node(0).get_global_data_view().scatter_allocated_write(alloc, data_str);
        co_await get_dedupe_node(0).get_global_data_view().sync(alloc);
        alloc_promise.set_value(std::move (alloc));
        co_return SUCCESS;
    };

    boost::asio::io_context ioc;
    boost::asio::co_spawn(ioc, write_data, boost::asio::detached);
    ioc.run();
    address alloc = alloc_promise.get_future().get();;

    char read_buf [data_size];

    auto read_data = [&] () -> coro <message_type> {
        size_t offset = 0;
        for (int i = 0; i < alloc.size(); ++i) {
            const auto frag = alloc.get_fragment(i);
            co_await get_dedupe_node(0).get_global_data_view().read(read_buf + offset, frag.pointer, frag.size);
            offset += frag.size;
            if (offset == data_size) {
                break;
            }
        }
        co_return SUCCESS;
    };

    ioc.stop();
    ioc.restart();
    boost::asio::co_spawn(ioc, read_data, boost::asio::detached);
    ioc.run();

    BOOST_CHECK(std::string_view (data, data_size) == std::string_view (read_buf, data_size));

}

} // end namespace uh::cluster
