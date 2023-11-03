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
    BOOST_FIXTURE_TEST_CASE (success_ec, cluster_fixture) {
        BOOST_TEST(true);
    }


/*
BOOST_FIXTURE_TEST_CASE (ec_basic_write_read_test_multiple_nodes_with_ec, cluster_fixture)
{
    //std::cout << "begin ec_basic_write_read_test_multiple_nodes_with_ec" << std::endl;
    setup(4, 1, 0, XOR);
    BOOST_TEST (get_dedupe_node(0).get_global_data_view().get_data_node_count() == 3);

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
    boost::asio::co_spawn(ioc, write_data, boost::asio::use_future);
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
    boost::asio::co_spawn(ioc, read_data, boost::asio::use_future);
    ioc.run();

    BOOST_CHECK(std::string_view (data, data_size) == std::string_view (read_buf, data_size));
    //std::cout << "end ec_basic_write_read_test_multiple_nodes_with_ec" << std::endl;
}

BOOST_FIXTURE_TEST_CASE (ec_basic_write_read_test_single_node_without_ec, cluster_fixture)
{
    //std::cout << "begin ec_basic_write_read_test_single_node_without_ec" << std::endl;
    setup(1, 1, 0, NON);

    BOOST_TEST (get_dedupe_node(0).get_global_data_view().get_data_node_count() == 1);
    constexpr auto data_size = 3ul*1024ul;
    char data [data_size];

    fill_random(data, data_size);

    std::promise <address> alloc_promise;
    const auto data_str = std::string_view (data, data_size);
    auto write_data = [&] () -> coro <message_type> {
        //std::cout << "before alloc" << std::endl;
        auto alloc = co_await get_dedupe_node(0).get_global_data_view().allocate(data_size);
        //std::cout << "before alloc write" << std::endl;
        co_await get_dedupe_node(0).get_global_data_view().scatter_allocated_write(alloc, data_str);
        //std::cout << "before sync" << std::endl;
        co_await get_dedupe_node(0).get_global_data_view().sync(alloc);
        //std::cout << "after sync" << std::endl;
        alloc_promise.set_value(std::move (alloc));
        co_return SUCCESS;
    };

    boost::asio::io_context ioc;
    boost::asio::co_spawn(ioc, write_data, boost::asio::use_future);
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
    boost::asio::co_spawn(ioc, read_data, boost::asio::use_future);
    ioc.run();

    BOOST_CHECK(std::string_view (data, data_size) == std::string_view (read_buf, data_size));
    //std::cout << "end ec_basic_write_read_test_single_node_without_ec" << std::endl;

}

BOOST_FIXTURE_TEST_CASE (ec_basic_write_read_test_multiple_nodes_without_ec, cluster_fixture)
{
    //std::cout << "begin ec_basic_write_read_test_multiple_nodes_without_ec" << std::endl;

    setup(7, 1, 0, XOR);
    BOOST_TEST (get_dedupe_node(0).get_global_data_view().get_data_node_count() == 6);
    constexpr auto data_size = 12ul*1024ul;
    char data [data_size];

    fill_random(data, data_size);

    std::promise <address> alloc_promise;
    const auto data_str = std::string_view (data, data_size);
    auto write_data = [&] () -> coro <message_type> {
        //std::cout << "before alloc" << std::endl;
        auto alloc = co_await get_dedupe_node(0).get_global_data_view().allocate(data_size);
        //std::cout << "before scatter" << std::endl;
        co_await get_dedupe_node(0).get_global_data_view().scatter_allocated_write(alloc, data_str);
        //std::cout << "before sync" << std::endl;
        co_await get_dedupe_node(0).get_global_data_view().sync(alloc);
        //std::cout << "after sync" << std::endl;
        alloc_promise.set_value(std::move (alloc));
        co_return SUCCESS;
    };

    boost::asio::io_context ioc;
    boost::asio::co_spawn(ioc, write_data, boost::asio::use_future);
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
    boost::asio::co_spawn(ioc, read_data, boost::asio::use_future);
    ioc.run();

    BOOST_CHECK(std::string_view (data, data_size) == std::string_view (read_buf, data_size));
    //std::cout << "end ec_basic_write_read_test_multiple_nodes_without_ec" << std::endl;

}

BOOST_FIXTURE_TEST_CASE (ec_exception_test_single_node_with_ec, cluster_fixture)
{
    //std::cout << "begin ec_exception_test_single_node_with_ec" << std::endl;

    BOOST_CHECK_THROW (setup(1, 1, 0, XOR), std::exception);
    //std::cout << "end ec_exception_test_single_node_with_ec" << std::endl;

}

BOOST_FIXTURE_TEST_CASE (ec_exception_test_non_divisible_data_size, cluster_fixture)
{
    //std::cout << "begin ec_exception_test_non_divisible_data_size" << std::endl;

    setup(7, 1, 0, XOR);
    BOOST_TEST (get_dedupe_node(0).get_global_data_view().get_data_node_count() == 6);
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
    //std::cout << "end ec_exception_test_non_divisible_data_size" << std::endl;

}

BOOST_FIXTURE_TEST_CASE (ec_test_lost_one_data_node_without_recover, cluster_fixture)
{
    //std::cout << "begin ec_test_lost_one_data_node_without_recover" << std::endl;

    setup(12, 1, 0, XOR);
    BOOST_TEST (get_dedupe_node(0).get_global_data_view().get_data_node_count() == 11);
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
    boost::asio::co_spawn(ioc, write_data, boost::asio::use_future);
    ioc.run();

    address alloc = alloc_promise.get_future().get();;

    ioc.stop();
    ioc.restart();
    shut_down();
    destroy_data_node(3);
    turn_on(12, 1, 0, XOR);
    BOOST_TEST (get_dedupe_node(0).get_global_data_view().get_data_node_count() == 11);

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

    boost::asio::co_spawn(ioc, read_data, boost::asio::use_future);
    ioc.run();

    BOOST_CHECK(std::string_view (data, data_size) != std::string_view (read_buf, data_size));
    //std::cout << "end ec_test_lost_one_data_node_without_recover" << std::endl;

}

BOOST_FIXTURE_TEST_CASE (ec_test_lost_one_data_node_with_recover, cluster_fixture)
{
    //std::cout << "begin ec_test_lost_one_data_node_with_recover" << std::endl;

    setup(12, 1, 0, XOR);
    BOOST_TEST (get_dedupe_node(0).get_global_data_view().get_data_node_count() == 11);

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
    boost::asio::co_spawn(ioc, write_data, boost::asio::use_future);
    ioc.run();

    address alloc = alloc_promise.get_future().get();;

    ioc.stop();
    ioc.restart();
    shut_down();
    destroy_data_node(3);
    turn_on(12, 1, 0, XOR);
    BOOST_TEST (get_dedupe_node(0).get_global_data_view().get_data_node_count() == 11);

    boost::asio::co_spawn (ioc, [&] () -> coro <void> {co_await get_dedupe_node(0).get_global_data_view().recover();}, boost::asio::use_future);
    ioc.run();
    ioc.stop();
    ioc.restart();

    char read_buf [data_size];

    auto read_data = [&] () -> coro <message_type> {
        size_t offset = 0;
        for (int i = 0; i < alloc.size(); ++i) {
            const auto frag = alloc.get_fragment(i);
            const auto s = co_await get_dedupe_node(0).get_global_data_view().read(read_buf + offset, frag.pointer, frag.size);
            if (s != frag.size) {
                int i = 0;
            }
            offset += frag.size;
            if (offset == data_size) {
                break;
            }
        }
        co_return SUCCESS;
    };

    boost::asio::co_spawn(ioc, read_data, boost::asio::use_future);
    ioc.run();

    BOOST_CHECK(std::string_view (data, data_size) == std::string_view (read_buf, data_size));
    //std::cout << "end ec_test_lost_one_data_node_with_recover" << std::endl;

}

BOOST_FIXTURE_TEST_CASE (ec_test_lost_one_data_node_with_recover_no_ec, cluster_fixture)
{
    //std::cout << "begin ec_test_lost_one_data_node_with_recover_no_ec" << std::endl;

    setup(14, 1, 0, NON);
    BOOST_TEST (get_dedupe_node(0).get_global_data_view().get_data_node_count() == 14);

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
    boost::asio::co_spawn(ioc, write_data, boost::asio::use_future);
    ioc.run();

    address alloc = alloc_promise.get_future().get();;

    ioc.stop();
    ioc.restart();
    shut_down();
    destroy_data_node(3);
    turn_on(14, 1, 0, NON);
    BOOST_TEST (get_dedupe_node(0).get_global_data_view().get_data_node_count() == 14);

    boost::asio::co_spawn (ioc, [&] () -> coro <void> {co_await get_dedupe_node(0).get_global_data_view().recover();}, boost::asio::use_future);
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

    boost::asio::co_spawn(ioc, read_data, boost::asio::use_future);
    ioc.run();

    BOOST_CHECK(std::string_view (data, data_size) != std::string_view (read_buf, data_size));
    //std::cout << "end ec_test_lost_one_data_node_with_recover_no_ec" << std::endl;

}


BOOST_FIXTURE_TEST_CASE (ec_test_lost_no_data_node_with_recover, cluster_fixture)
{
    //std::cout << "begin ec_test_lost_no_data_node_with_recover" << std::endl;

    setup(15, 1, 0, XOR);
    BOOST_TEST (get_dedupe_node(0).get_global_data_view().get_data_node_count() == 14);

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
    boost::asio::co_spawn(ioc, write_data, boost::asio::use_future);
    ioc.run();

    address alloc = alloc_promise.get_future().get();;

    ioc.stop();
    ioc.restart();
    shut_down();
    turn_on(15, 1, 0, XOR);
    BOOST_TEST (get_dedupe_node(0).get_global_data_view().get_data_node_count() == 14);

    boost::asio::co_spawn (ioc, [&] () -> coro <void> {co_await get_dedupe_node(0).get_global_data_view().recover();}, boost::asio::use_future);
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

    boost::asio::co_spawn(ioc, read_data, boost::asio::use_future);
    ioc.run();

    BOOST_CHECK(std::string_view (data, data_size) == std::string_view (read_buf, data_size));
    //std::cout << "end ec_test_lost_no_data_node_with_recover" << std::endl;
}

BOOST_FIXTURE_TEST_CASE (ec_test_lost_no_data_node_with_recover_no_ec, cluster_fixture)
{
    //std::cout << "begin ec_test_lost_no_data_node_with_recover_no_ec" << std::endl;
    setup(20, 1, 0, NON);
    BOOST_TEST (get_dedupe_node(0).get_global_data_view().get_data_node_count() == 20);

    std::promise <address> alloc_promise;
    constexpr auto data_size = 20ul*1024ul;
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
    boost::asio::co_spawn(ioc, write_data, boost::asio::use_future);
    ioc.run();

    address alloc = alloc_promise.get_future().get();;

    ioc.stop();
    ioc.restart();
    shut_down();
    turn_on(20, 1, 0, NON);
    BOOST_TEST (get_dedupe_node(0).get_global_data_view().get_data_node_count() == 20);

    boost::asio::co_spawn (ioc, [&] () -> coro <void> {co_await get_dedupe_node(0).get_global_data_view().recover();}, boost::asio::use_future);
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

    boost::asio::co_spawn(ioc, read_data, boost::asio::use_future);
    ioc.run();

    BOOST_CHECK(std::string_view (data, data_size) == std::string_view (read_buf, data_size));
    //std::cout << "end ec_test_lost_no_data_node_with_recover_no_ec" << std::endl;

}

BOOST_FIXTURE_TEST_CASE (ec_test_lost_two_data_nodes_with_recover, cluster_fixture)
{
    //std::cout << "begin ec_test_lost_two_data_nodes_with_recover" << std::endl;

    setup(11, 1, 0, XOR);
    BOOST_TEST (get_dedupe_node(0).get_global_data_view().get_data_node_count() == 10);

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
    boost::asio::co_spawn(ioc, write_data, boost::asio::use_future);
    ioc.run();

    address alloc = alloc_promise.get_future().get();;

    ioc.stop();
    ioc.restart();
    shut_down();
    destroy_data_node(2);
    destroy_data_node(5);

    turn_on(11, 1, 0, XOR);
    BOOST_TEST (get_dedupe_node(0).get_global_data_view().get_data_node_count() == 10);

    boost::asio::co_spawn (ioc, [&] () -> coro <void> {co_await get_dedupe_node(0).get_global_data_view().recover();},
                       [] (const std::exception_ptr& e) {
                            if (e) {
                               try {std::rethrow_exception(e);
                           } catch (std::exception& e) {throw;}
                        }
    });

    BOOST_CHECK_THROW (ioc.run(), std::runtime_error);
    //std::cout << "end ec_test_lost_two_data_nodes_with_recover" << std::endl;

}

BOOST_FIXTURE_TEST_CASE (ec_test_lost_one_data_node_with_recover_non_dividable_to_unsigne_long_size, cluster_fixture)
{
    //std::cout << "begin ec_test_lost_one_data_node_with_recover_non_dividable_to_unsigne_long_size" << std::endl;

    setup(19, 1, 0, XOR);
    BOOST_TEST (get_dedupe_node(0).get_global_data_view().get_data_node_count() == 18);

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
    boost::asio::co_spawn(ioc, write_data, boost::asio::use_future);
    ioc.run();

    address alloc = alloc_promise.get_future().get();;

    ioc.stop();
    ioc.restart();
    shut_down();
    destroy_data_node(6);
    turn_on(19, 1, 0, XOR);
    BOOST_TEST (get_dedupe_node(0).get_global_data_view().get_data_node_count() == 18);

    boost::asio::co_spawn (ioc, [&] () -> coro <void> {co_await get_dedupe_node(0).get_global_data_view().recover();}, boost::asio::use_future);
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

    boost::asio::co_spawn(ioc, read_data, boost::asio::use_future);
    ioc.run();

    BOOST_CHECK(std::string_view (data, data_size) == std::string_view (read_buf, data_size));
    //std::cout << "end ec_test_lost_one_data_node_with_recover_non_dividable_to_unsigne_long_size" << std::endl;

}

BOOST_FIXTURE_TEST_CASE (ec_test_empty_nodes, cluster_fixture)
{
    //std::cout << "begin ec_test_empty_nodes" << std::endl;

    setup(25, 1, 0, XOR);
    BOOST_TEST (get_dedupe_node(0).get_global_data_view().get_data_node_count() == 24);


    shut_down();
    turn_on(25, 1, 0, XOR);
    BOOST_TEST (get_dedupe_node(0).get_global_data_view().get_data_node_count() == 24);
    boost::asio::io_context ioc;

    boost::asio::co_spawn (ioc, [&] () -> coro <void> {co_await get_dedupe_node(0).get_global_data_view().recover();}, boost::asio::use_future);
    ioc.run();
    //std::cout << "end ec_test_empty_nodes" << std::endl;

}

BOOST_FIXTURE_TEST_CASE (ec_test_empty_nodes_no_ec, cluster_fixture)
{
    //std::cout << "begin ec_test_empty_nodes_no_ec" << std::endl;

    setup(25, 1, 0, NON);
    BOOST_TEST (get_dedupe_node(0).get_global_data_view().get_data_node_count() == 25);

    shut_down();
    turn_on(25, 1, 0, NON);
    BOOST_TEST (get_dedupe_node(0).get_global_data_view().get_data_node_count() == 25);
    boost::asio::io_context ioc;

    boost::asio::co_spawn (ioc, [&] () -> coro <void> {co_await get_dedupe_node(0).get_global_data_view().recover();}, boost::asio::use_future);
    ioc.run();
    //std::cout << "end ec_test_empty_nodes_no_ec" << std::endl;

}

BOOST_FIXTURE_TEST_CASE (ec_test_chain_of_failures, cluster_fixture)
{
    //std::cout << "begin ec_test_chain_of_failures" << std::endl;

    setup(30, 1, 0, XOR);
    BOOST_TEST (get_dedupe_node(0).get_global_data_view().get_data_node_count() == 29);

    std::promise <address> alloc_promise;
    constexpr auto data_size = 29ul * 6ul*1024ul + 29ul * 100;
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
    boost::asio::co_spawn(ioc, write_data, boost::asio::use_future);
    ioc.run();

    address alloc = alloc_promise.get_future().get();;

    ioc.stop();
    ioc.restart();
    shut_down();
    destroy_data_node(0);
    turn_on(30, 1, 0, XOR);
    BOOST_TEST (get_dedupe_node(0).get_global_data_view().get_data_node_count() == 29);

    boost::asio::co_spawn (ioc, [&] () -> coro <void> {co_await get_dedupe_node(0).get_global_data_view().recover();}, boost::asio::use_future);
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

    boost::asio::co_spawn(ioc, read_data, boost::asio::use_future);
    ioc.run();

    BOOST_CHECK(std::string_view (data, data_size) == std::string_view (read_buf, data_size));

    ioc.stop();
    ioc.restart();
    shut_down();
    destroy_data_node(30);
    turn_on(30, 1, 0, XOR);
    BOOST_TEST (get_dedupe_node(0).get_global_data_view().get_data_node_count() == 29);

    boost::asio::co_spawn (ioc, [&] () -> coro <void> {co_await get_dedupe_node(0).get_global_data_view().recover();}, boost::asio::use_future);
    ioc.run();
    ioc.stop();
    ioc.restart();

 // second write

    std::promise <address> alloc_promise_2;
    constexpr auto data_size_2 = 29ul * 2ul*1024ul + 29ul * 200;
    char data_2 [data_size_2];
    fill_random(data_2, data_size_2);

    const auto data_str_2 = std::string_view (data_2, data_size_2);
    auto write_data_2 = [&] () -> coro <void> {
        auto alloc = co_await get_dedupe_node(0).get_global_data_view().allocate(data_size_2);
        co_await get_dedupe_node(0).get_global_data_view().scatter_allocated_write(alloc, data_str_2);
        co_await get_dedupe_node(0).get_global_data_view().sync(alloc);
        alloc_promise_2.set_value(std::move (alloc));
        co_return;
    };

    boost::asio::co_spawn(ioc, write_data_2, boost::asio::use_future);
    ioc.run();

    address alloc_2 = alloc_promise_2.get_future().get();

    ioc.stop();
    ioc.restart();
    shut_down();
    destroy_data_node(0);
    turn_on(30, 1, 0, XOR);
    BOOST_TEST (get_dedupe_node(0).get_global_data_view().get_data_node_count() == 29);

    boost::asio::co_spawn (ioc, [&] () -> coro <void> {co_await get_dedupe_node(0).get_global_data_view().recover();}, boost::asio::use_future);
    ioc.run();
    ioc.stop();
    ioc.restart();

    // third write

    std::promise <address> alloc_promise_3;
    constexpr auto data_size_3 = 29ul * 3ul*1024ul + 29ul * 400;
    char data_3 [data_size_3];
    fill_random(data_3, data_size_3);

    const auto data_str_3 = std::string_view (data_3, data_size_3);
    auto write_data_3 = [&] () -> coro <void> {
        auto alloc = co_await get_dedupe_node(0).get_global_data_view().allocate(data_size_3);
        co_await get_dedupe_node(0).get_global_data_view().scatter_allocated_write(alloc, data_str_3);
        co_await get_dedupe_node(0).get_global_data_view().sync(alloc);
        alloc_promise_3.set_value(std::move (alloc));
        co_return;
    };
    boost::asio::co_spawn(ioc, write_data_3, boost::asio::use_future);
    ioc.run();

    address alloc_3 = alloc_promise_3.get_future().get();

    ioc.stop();
    ioc.restart();
    shut_down();
    destroy_data_node(0);
    turn_on(30, 1, 0, XOR);
    BOOST_TEST (get_dedupe_node(0).get_global_data_view().get_data_node_count() == 29);

    boost::asio::co_spawn (ioc, [&] () -> coro <void> {co_await get_dedupe_node(0).get_global_data_view().recover();}, boost::asio::use_future);
    ioc.run();
    ioc.stop();
    ioc.restart();

    char read_buf_1 [data_size];
    char read_buf_2 [data_size_2];
    char read_buf_3 [data_size_3];

    auto read_data_1 = [&] () -> coro <message_type> {
        size_t offset = 0;
        for (int i = 0; i < alloc.size(); ++i) {
            const auto frag = alloc.get_fragment(i);
            co_await get_dedupe_node(0).get_global_data_view().read(read_buf_1 + offset, frag.pointer, frag.size);
            offset += frag.size;
            if (offset == data_size) {
                break;
            }
        }
        co_return SUCCESS;
    };

    boost::asio::co_spawn(ioc, read_data_1, boost::asio::use_future);
    ioc.run();
    ioc.stop();
    ioc.restart();

    auto read_data_2 = [&] () -> coro <message_type> {
        size_t offset = 0;
        for (int i = 0; i < alloc_2.size(); ++i) {
            const auto frag = alloc_2.get_fragment(i);
            co_await get_dedupe_node(0).get_global_data_view().read(read_buf_2 + offset, frag.pointer, frag.size);
            offset += frag.size;
            if (offset == data_size_2) {
                break;
            }
        }
        co_return SUCCESS;
    };

    boost::asio::co_spawn(ioc, read_data_2, boost::asio::use_future);
    ioc.run();
    ioc.stop();
    ioc.restart();

    auto read_data_3 = [&] () -> coro <message_type> {
        size_t offset = 0;
        for (int i = 0; i < alloc_3.size(); ++i) {
            const auto frag = alloc_3.get_fragment(i);
            co_await get_dedupe_node(0).get_global_data_view().read(read_buf_3 + offset, frag.pointer, frag.size);
            offset += frag.size;
            if (offset == data_size_3) {
                break;
            }
        }
        co_return SUCCESS;
    };

    boost::asio::co_spawn(ioc, read_data_3, boost::asio::use_future);
    ioc.run();
    ioc.stop();
    ioc.restart();

    BOOST_CHECK(std::string_view (data, data_size) == std::string_view (read_buf_1, data_size));
    BOOST_CHECK(std::string_view (data_2, data_size_2) == std::string_view (read_buf_2, data_size_2));
    BOOST_CHECK(std::string_view (data_3, data_size_3) == std::string_view (read_buf_3, data_size_3));

    //std::cout << "end ec_test_chain_of_failures" << std::endl;

}
*/
} // end namespace uh::cluster
