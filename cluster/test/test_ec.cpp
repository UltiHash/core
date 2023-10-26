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

#include "cluster_fixture.h"



// ------------- Tests Suites Follow --------------

namespace uh::cluster {


void fill_random(char* buf, size_t size) {
    for (int i = 0; i < size; ++i) {
        buf[i] = rand()&0xff;
    }
}

BOOST_FIXTURE_TEST_CASE (basic_write_read_test_multiple_nodes_with_ec, cluster_fixture)
{
    setup(4, 1, 0, XOR);

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

BOOST_FIXTURE_TEST_CASE (basic_write_read_test_single_node_without_ec, cluster_fixture)
{
    setup(1, 1, 0, NON);

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

BOOST_FIXTURE_TEST_CASE (basic_write_read_test_multiple_nodes_without_ec, cluster_fixture)
{
    setup(7, 1, 0, XOR);

    constexpr auto data_size = 12ul*1024ul;
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

BOOST_FIXTURE_TEST_CASE (exception_test_single_node_with_ec, cluster_fixture)
{
    BOOST_CHECK_THROW (setup(1, 1, 0, XOR), std::logic_error);
}

BOOST_FIXTURE_TEST_CASE (exception_test_non_divisible_data_size, cluster_fixture)
{
    setup(7, 1, 0, XOR);
    std::promise <address> alloc_promise;
    constexpr auto data_size = 14ul*1024ul;
    char data [data_size];

    const auto data_str = std::string_view (data, data_size);
    auto write_data = [&] () -> coro <void> {
        auto alloc = co_await get_dedupe_node(0).get_global_data_view().allocate(data_size);
        co_await get_dedupe_node(0).get_global_data_view().scatter_allocated_write(alloc, data_str);
        co_await get_dedupe_node(0).get_global_data_view().sync(alloc);
        alloc_promise.set_value(std::move (alloc));
        co_return;
    };

    std::exception_ptr excp_ptr;
    boost::asio::io_context ioc;
    boost::asio::co_spawn(ioc, write_data, [&] (std::exception_ptr e) {if (e) {excp_ptr = std::move (e);}});
    ioc.run();
    BOOST_CHECK_THROW (std::rethrow_exception(excp_ptr), std::length_error);
}

BOOST_FIXTURE_TEST_CASE (ec_test_lost_one_data_node_without_recover, cluster_fixture)
{
    setup(12, 1, 0, XOR);
    std::promise <address> alloc_promise;
    constexpr auto data_size = 11ul*1024ul;
    char data [data_size];
    fill_random(data, data_size);

    const auto data_str = std::string_view (data, data_size);
    auto write_data = [&] () -> coro <void> {
        auto alloc = co_await get_dedupe_node(0).get_global_data_view().allocate(data_size);
        co_await get_dedupe_node(0).get_global_data_view().scatter_allocated_write(alloc, data_str);
        co_await get_dedupe_node(0).get_global_data_view().sync(alloc);
        alloc_promise.set_value(std::move (alloc));
        co_return;
    };
    boost::asio::io_context ioc;
    boost::asio::co_spawn(ioc, write_data, boost::asio::detached);
    ioc.run();

    address alloc = alloc_promise.get_future().get();;

    ioc.stop();
    ioc.restart();
    shut_down();
    destroy_data_node(3);
    turn_on(12, 1, 0, XOR);

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

    boost::asio::co_spawn(ioc, read_data, boost::asio::detached);
    ioc.run();

    BOOST_CHECK(std::string_view (data, data_size) != std::string_view (read_buf, data_size));

}

BOOST_FIXTURE_TEST_CASE (ec_test_lost_one_data_node_with_recover, cluster_fixture)
{
    setup(12, 1, 0, XOR);
    std::promise <address> alloc_promise;
    constexpr auto data_size = 11ul*1024ul;
    char data [data_size];
    fill_random(data, data_size);

    const auto data_str = std::string_view (data, data_size);
    auto write_data = [&] () -> coro <void> {
        auto alloc = co_await get_dedupe_node(0).get_global_data_view().allocate(data_size);
        co_await get_dedupe_node(0).get_global_data_view().scatter_allocated_write(alloc, data_str);
        co_await get_dedupe_node(0).get_global_data_view().sync(alloc);
        alloc_promise.set_value(std::move (alloc));
        co_return;
    };
    boost::asio::io_context ioc;
    boost::asio::co_spawn(ioc, write_data, boost::asio::detached);
    ioc.run();

    address alloc = alloc_promise.get_future().get();;

    ioc.stop();
    ioc.restart();
    shut_down();
    destroy_data_node(3);
    turn_on(12, 1, 0, XOR);

    boost::asio::co_spawn (ioc, [&] () -> coro <void> {co_await get_dedupe_node(0).get_global_data_view().recover();}, boost::asio::detached);
    ioc.run();
    ioc.stop();
    ioc.restart();

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

    boost::asio::co_spawn(ioc, read_data, boost::asio::detached);
    ioc.run();

    BOOST_CHECK(std::string_view (data, data_size) == std::string_view (read_buf, data_size));

}

BOOST_FIXTURE_TEST_CASE (ec_test_lost_one_data_node_with_recover_no_ec, cluster_fixture)
{
    setup(14, 1, 0, NON);
    std::promise <address> alloc_promise;
    constexpr auto data_size = 14ul*1024ul;
    char data [data_size];
    fill_random(data, data_size);

    const auto data_str = std::string_view (data, data_size);
    auto write_data = [&] () -> coro <void> {
        auto alloc = co_await get_dedupe_node(0).get_global_data_view().allocate(data_size);
        co_await get_dedupe_node(0).get_global_data_view().scatter_allocated_write(alloc, data_str);
        co_await get_dedupe_node(0).get_global_data_view().sync(alloc);
        alloc_promise.set_value(std::move (alloc));
        co_return;
    };
    boost::asio::io_context ioc;
    boost::asio::co_spawn(ioc, write_data, boost::asio::detached);
    ioc.run();

    address alloc = alloc_promise.get_future().get();;

    ioc.stop();
    ioc.restart();
    shut_down();
    destroy_data_node(3);
    turn_on(14, 1, 0, NON);

    boost::asio::co_spawn (ioc, [&] () -> coro <void> {co_await get_dedupe_node(0).get_global_data_view().recover();}, boost::asio::detached);
    ioc.run();
    ioc.stop();
    ioc.restart();

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

    boost::asio::co_spawn(ioc, read_data, boost::asio::detached);
    ioc.run();

    BOOST_CHECK(std::string_view (data, data_size) != std::string_view (read_buf, data_size));

}


BOOST_FIXTURE_TEST_CASE (ec_test_lost_no_data_node_with_recover, cluster_fixture)
{
    setup(15, 1, 0, XOR);
    std::promise <address> alloc_promise;
    constexpr auto data_size = 14ul*1024ul;
    char data [data_size];
    fill_random(data, data_size);

    const auto data_str = std::string_view (data, data_size);
    auto write_data = [&] () -> coro <void> {
        auto alloc = co_await get_dedupe_node(0).get_global_data_view().allocate(data_size);
        co_await get_dedupe_node(0).get_global_data_view().scatter_allocated_write(alloc, data_str);
        co_await get_dedupe_node(0).get_global_data_view().sync(alloc);
        alloc_promise.set_value(std::move (alloc));
        co_return;
    };
    boost::asio::io_context ioc;
    boost::asio::co_spawn(ioc, write_data, boost::asio::detached);
    ioc.run();

    address alloc = alloc_promise.get_future().get();;

    ioc.stop();
    ioc.restart();
    shut_down();
    turn_on(15, 1, 0, XOR);

    boost::asio::co_spawn (ioc, [&] () -> coro <void> {co_await get_dedupe_node(0).get_global_data_view().recover();}, boost::asio::detached);
    ioc.run();
    ioc.stop();
    ioc.restart();

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

    boost::asio::co_spawn(ioc, read_data, boost::asio::detached);
    ioc.run();

    BOOST_CHECK(std::string_view (data, data_size) == std::string_view (read_buf, data_size));

}

BOOST_FIXTURE_TEST_CASE (ec_test_lost_two_data_nodes_with_recover, cluster_fixture)
{
    setup(11, 1, 0, XOR);
    std::promise <address> alloc_promise;
    constexpr auto data_size = 10ul*1024ul;
    char data [data_size];
    fill_random(data, data_size);

    const auto data_str = std::string_view (data, data_size);
    auto write_data = [&] () -> coro <void> {
        auto alloc = co_await get_dedupe_node(0).get_global_data_view().allocate(data_size);
        co_await get_dedupe_node(0).get_global_data_view().scatter_allocated_write(alloc, data_str);
        co_await get_dedupe_node(0).get_global_data_view().sync(alloc);
        alloc_promise.set_value(std::move (alloc));
        co_return;
    };
    boost::asio::io_context ioc;
    boost::asio::co_spawn(ioc, write_data, boost::asio::detached);
    ioc.run();

    address alloc = alloc_promise.get_future().get();;

    ioc.stop();
    ioc.restart();
    shut_down();
    destroy_data_node(2);
    destroy_data_node(5);

    turn_on(11, 1, 0, XOR);

    boost::asio::co_spawn (ioc, [&] () -> coro <void> {co_await get_dedupe_node(0).get_global_data_view().recover();},
                       [] (const std::exception_ptr& e) {
                            if (e) {
                               try {std::rethrow_exception(e);
                           } catch (std::exception& e) {throw;}
                        }
    });

    BOOST_CHECK_THROW (ioc.run(), std::runtime_error);

}

BOOST_FIXTURE_TEST_CASE (ec_test_lost_one_data_node_with_recover_non_dividable_to_unsigne_long_size, cluster_fixture)
{
    setup(19, 1, 0, XOR);
    std::promise <address> alloc_promise;
    constexpr auto data_size = 18ul*1037ul;
    char data [data_size];
    fill_random(data, data_size);

    const auto data_str = std::string_view (data, data_size);
    auto write_data = [&] () -> coro <void> {
        auto alloc = co_await get_dedupe_node(0).get_global_data_view().allocate(data_size);
        co_await get_dedupe_node(0).get_global_data_view().scatter_allocated_write(alloc, data_str);
        co_await get_dedupe_node(0).get_global_data_view().sync(alloc);
        alloc_promise.set_value(std::move (alloc));
        co_return;
    };
    boost::asio::io_context ioc;
    boost::asio::co_spawn(ioc, write_data, boost::asio::detached);
    ioc.run();

    address alloc = alloc_promise.get_future().get();;

    ioc.stop();
    ioc.restart();
    shut_down();
    destroy_data_node(6);
    turn_on(19, 1, 0, XOR);

    boost::asio::co_spawn (ioc, [&] () -> coro <void> {co_await get_dedupe_node(0).get_global_data_view().recover();}, boost::asio::detached);
    ioc.run();
    ioc.stop();
    ioc.restart();

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

    boost::asio::co_spawn(ioc, read_data, boost::asio::detached);
    ioc.run();

    BOOST_CHECK(std::string_view (data, data_size) == std::string_view (read_buf, data_size));

}

} // end namespace uh::cluster
