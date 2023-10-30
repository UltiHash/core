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

#include "cluster_fixture.h"


// ------------- Tests Suites Follow --------------

namespace uh::cluster {

// ---------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE (test_uncached_write, cluster_fixture)
{
    setup (3, 1, 0, XOR);

    global_data_view& data_view = get_dedupe_node(0).get_global_data_view();
    std::string data_in = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Maecenas risus justo, blandit tincidunt hendrerit ac, pulvinar sed quam.";

    std::promise<address> write_result_promise;
    auto write_data = [&]() -> boost::asio::awaitable<void> {
        write_result_promise.set_value(co_await data_view.write(data_in));
    };
    boost::asio::io_context ioc;
    boost::asio::co_spawn(ioc, write_data, boost::asio::use_future);
    ioc.run();
    address write_result = write_result_promise.get_future().get();

    ospan<char> read_result(data_in.size());
    std::promise<std::size_t> read_result_promise;
    auto read_data = [&]() -> boost::asio::awaitable<void> {
        size_t read_count = 0;
        for(int i = 0; i < write_result.size(); i++) {
            auto frag = write_result.get_fragment(i);
            co_await data_view.read(read_result.data.get() + read_count, frag.pointer, frag.size);
            read_count += frag.size;
        }
        read_result_promise.set_value(read_count);
    };
    ioc.stop();
    ioc.restart();
    boost::asio::co_spawn(ioc, read_data, boost::asio::use_future);
    ioc.run();
    read_result_promise.get_future().get();

    std::string_view data_out(read_result.data.get(), read_result.size);

    BOOST_CHECK(data_out == data_in);
}

BOOST_FIXTURE_TEST_CASE (test_cached_write_basic, cluster_fixture)
{
    setup (3, 1, 0, XOR);

    dedupe_config conf = make_dedupe_node_config(0);
    global_data_view& data_view = get_dedupe_node(0).get_global_data_view();

    std::string data_in_str = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Maecenas risus justo, blandit tincidunt hendrerit ac, pulvinar sed quam.";
    std::string_view data_in(data_in_str);
    dedupe_write_cache cache(data_in, data_view, conf);


    std::promise<address> write_result_promise;
    auto write_data = [&]() -> boost::asio::awaitable<void> {
        write_result_promise.set_value(co_await cache.write(data_in));
        co_await cache.flush();
    };
    boost::asio::io_context ioc;
    boost::asio::co_spawn(ioc, write_data, boost::asio::use_future);
    ioc.run();
    address write_result = write_result_promise.get_future().get();

    ospan<char> read_result(data_in.size());
    std::promise<std::size_t> read_result_promise;
    auto read_data = [&]() -> boost::asio::awaitable<void> {
        size_t read_count = 0;
        for (int i = 0; i < write_result.size(); i++) {
            auto frag = write_result.get_fragment(i);
            co_await data_view.read(read_result.data.get() + read_count, frag.pointer, frag.size);
            read_count += frag.size;
        }
        read_result_promise.set_value(read_count);
    };
    ioc.stop();
    ioc.restart();
    boost::asio::co_spawn(ioc, read_data, boost::asio::use_future);
    ioc.run();

    read_result_promise.get_future().get();
    std::string_view data_out(read_result.data.get(), read_result.size);

    BOOST_CHECK(data_out == data_in);
}

BOOST_FIXTURE_TEST_CASE (test_cached_write_auto_flush_realloc, cluster_fixture)
{
    setup (3, 1, 0, XOR);

    dedupe_config conf = make_dedupe_node_config(0);
    global_data_view& data_view = get_dedupe_node(0).get_global_data_view();

    auto data_size = 2 * conf.write_cache_size_per_dn * data_view.get_data_node_count();
    std::unique_ptr<char[]> write_buf = std::make_unique<char[]>(data_size);

    fill_random(write_buf.get(), data_size);
    std::string_view data_in(write_buf.get(), data_size);
    dedupe_write_cache cache(data_in, data_view, conf);

    std::promise<address> write_result_promise;
    auto write_data = [&]() -> boost::asio::awaitable<void> {
        address addr;
        for(std::size_t i = 0; i < data_size; i += conf.max_fragment_size) {
            addr.append_address(co_await cache.write(data_in.substr(i, conf.max_fragment_size)));
        }
        write_result_promise.set_value(std::move(addr));
        co_await cache.flush();
    };
    boost::asio::io_context ioc;
    boost::asio::co_spawn(ioc, write_data, boost::asio::use_future);
    ioc.run();
    address write_result = write_result_promise.get_future().get();;


    std::unique_ptr<char[]> read_buf = std::make_unique<char[]>(data_size);
    std::promise<std::size_t> read_result_promise;
    auto read_data = [&]() -> boost::asio::awaitable<void> {
        size_t read_count = 0;
        for(int i = 0; i < write_result.size(); i++) {
            auto frag = write_result.get_fragment(i);
            co_await data_view.read(read_buf.get() + read_count, frag.pointer, frag.size);
            read_count += frag.size;
        }
        read_result_promise.set_value(read_count);
    };

    ioc.stop();
    ioc.restart();
    boost::asio::co_spawn(ioc, read_data, boost::asio::use_future);
    ioc.run();
    std::size_t read_count_result = read_result_promise.get_future().get();

    std::string_view data_out(read_buf.get(), read_count_result);

    BOOST_CHECK(data_out == data_in);
}

//BOOST_FIXTURE_TEST_CASE (test_cached_write_partition_overlap, cluster_fixture)
//{
//    setup (3, 1, 0, XOR);
//    //- is a fragment correctly splitted when spanning across cache partitions belonging to different DNs?
//}
//
//BOOST_FIXTURE_TEST_CASE (test_cached_write_fragmented_cache, cluster_fixture)
//{
//    setup (3, 1, 0, XOR);
//    //- when fragmentation occurs due to padding (split fragment < min frag size), the overall data that should be written may not fit into the cache any longer. Does reallocation work properly?
//}
// ---------------------------------------------------------------------

} // end namespace uh::cluster

