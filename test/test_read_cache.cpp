//
// Created by masi on 11/1/23.
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

    BOOST_FIXTURE_TEST_CASE (read_cache_basic_write_read_test_multiple_nodes_with_ec, cluster_fixture)
    {
        setup(4, 1, 1, NONE);
        BOOST_TEST (get_directory_service(0).get_global_data_view().get_data_node_count() == 4);

        constexpr auto data_size = 4ul*1024ul;
        char data [data_size];

        fill_random(data, data_size);

        std::promise <address> alloc_promise;
        const auto data_str = std::string_view (data, data_size);
        auto write_data = [&] () -> coro <message_type> {
            auto alloc = get_deduplicator_service(0).get_global_data_view().write(data_str);
            get_deduplicator_service(0).get_global_data_view().sync(alloc);
            alloc_promise.set_value(std::move (alloc));
            co_return SUCCESS;
        };

        boost::asio::io_context ioc;
        boost::asio::co_spawn(ioc, write_data, boost::asio::use_future);
        ioc.run();
        address alloc = alloc_promise.get_future().get();

        char read_buf [data_size];

        auto read_data = [&] () -> coro <message_type> {
            get_deduplicator_service(0).get_global_data_view().read_address(read_buf, alloc);
            co_return SUCCESS;
        };

        ioc.stop();
        ioc.restart();
        boost::asio::co_spawn(ioc, read_data, boost::asio::use_future);
        ioc.run();

        BOOST_CHECK(std::string_view (data, data_size) == std::string_view (read_buf, data_size));
    }
} // end namespace uh::cluster
