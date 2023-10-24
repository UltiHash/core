//
// Created by masi on 7/24/23.
//

#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "global data view tests"
#endif

#include <boost/test/unit_test.hpp>
#include <boost/asio/awaitable.hpp>

#include <filesystem>

#include "global_data_view_fixture.h"


// ------------- Tests Suites Follow --------------

namespace uh::cluster {

// ---------------------------------------------------------------------

    BOOST_FIXTURE_TEST_CASE (test_uncached_write, global_data_view_fixture)
    {
        boost::asio::io_context io_context;
        boost::asio::io_context io_context2;
        global_data_view& data_view = dd0->get_global_data_view();


        std::string data_in = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Maecenas risus justo, blandit tincidunt hendrerit ac, pulvinar sed quam.";
        //std::string_view lorem_view(data_in);
        //dedupe_config conf = dd0->m_cluster_map.m_cluster_conf.dedupe_node_conf;
        //dedupe_write_cache cache(lorem_view, data_view, conf);


        std::promise<address> write_result_promise;
        boost::asio::co_spawn(io_context, [&]() -> boost::asio::awaitable<void> {
            write_result_promise.set_value(co_await data_view.write(data_in));
        }, boost::asio::detached);
        io_context.run();
        address write_result = write_result_promise.get_future().get();

        ospan<char> read_result(data_in.size());
        std::promise<bool> read_result_promise;
        boost::asio::co_spawn(io_context2, [&]() -> boost::asio::awaitable<void> {
            size_t read_count = 0;
            for(int i = 0; i < write_result.size(); i++) {
                auto frag = write_result.get_fragment(i);
                co_await data_view.read(read_result.data.get() + read_count, frag.pointer, frag.size);
                read_count += frag.size;
            }
            read_result_promise.set_value(true);
        }, boost::asio::detached);
        io_context2.run();
        read_result_promise.get_future().get();

        std::string_view data_out(read_result.data.get(), read_result.size);

        BOOST_CHECK(data_out == data_in);
    }

    BOOST_FIXTURE_TEST_CASE (test_cached_write, global_data_view_fixture)
    {
        boost::asio::io_context io_context;
        boost::asio::io_context io_context2;
        global_data_view& data_view = dd0->get_global_data_view();


        std::string data_in_str = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Maecenas risus justo, blandit tincidunt hendrerit ac, pulvinar sed quam.";
        std::string_view data_in(data_in_str);
        dedupe_config conf = dd0->m_cluster_map.m_cluster_conf.dedupe_node_conf;
        dedupe_write_cache cache(data_in, data_view, conf);


        std::promise<address> write_result_promise;
        boost::asio::co_spawn(io_context, [&]() -> boost::asio::awaitable<void> {
            write_result_promise.set_value(co_await cache.write(data_in));
        }, boost::asio::detached);
        io_context.run();
        address write_result = write_result_promise.get_future().get();

        ospan<char> read_result(data_in.size());
        std::promise<bool> read_result_promise;
        boost::asio::co_spawn(io_context2, [&]() -> boost::asio::awaitable<void> {
            size_t read_count = 0;
            for(int i = 0; i < write_result.size(); i++) {
                auto frag = write_result.get_fragment(i);
                co_await data_view.read(read_result.data.get() + read_count, frag.pointer, frag.size);
                read_count += frag.size;
            }
            read_result_promise.set_value(true);
        }, boost::asio::detached);
        io_context2.run();
        read_result_promise.get_future().get();

        std::string_view data_out(read_result.data.get(), read_result.size);

        BOOST_CHECK(data_out == data_in);
    }


// ---------------------------------------------------------------------

} // end namespace uh::cluster

