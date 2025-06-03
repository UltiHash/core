#define BOOST_TEST_MODULE "global data view tests"

#include <common/utils/strings.h>

#include <util/gdv_fixture.h>
#include <util/random.h>

#include <boost/test/unit_test.hpp>

// ------------- Tests Suites Follow --------------

namespace uh::cluster {

BOOST_FIXTURE_TEST_CASE(invalid_read, global_data_view_fixture) {
    auto gdv = get_data_view();
    BOOST_REQUIRE_THROW(
        boost::asio::co_spawn(
            get_executor(),
            gdv->read(
#if defined(WITH_EC)
                ((static_cast<uint128_t>(std::numeric_limits<uint64_t>::max()) +
                  1) *
                 get_group_config().data_shards) -
                    1,
#else
                std::numeric_limits<uint64_t>::max(),
#endif
                8 * KIBI_BYTE),
            boost::asio::use_future)
            .get(),
        std::exception);
}

BOOST_FIXTURE_TEST_CASE(valid_write_read_fragment, global_data_view_fixture) {
    auto gdv = get_data_view();
    // auto input_buffer = unique_buffer<char>(8 * KIBI_BYTE);
    // fill_random(input_buffer.data(), input_buffer.size());
    auto input_buffer = random_buffer(64);

    std::cout << "start writing" << std::endl;
    auto addr = boost::asio::co_spawn(
                    get_executor(), gdv->write(input_buffer.string_view(), {0}),
                    boost::asio::use_future)
                    .get();
    BOOST_TEST(input_buffer.size() == addr.data_size());
    BOOST_TEST(addr.fragments.size() == 1ul);

    std::cout << "start reading" << std::endl;
    unique_buffer<char> result_buffer(addr.data_size());
    boost::asio::co_spawn(get_executor(),
                          gdv->read_address(addr, result_buffer.span()),
                          boost::asio::use_future)
        .get();
    BOOST_TEST(input_buffer.string_view() == result_buffer.string_view());
}

BOOST_FIXTURE_TEST_CASE(invalid_read_address, global_data_view_fixture) {

    auto gdv = get_data_view();
    address addr;
    addr.push({std::numeric_limits<uint64_t>::max(), 42});
    auto result_buffer = unique_buffer<char>(addr.data_size());

    BOOST_REQUIRE_THROW(
        boost::asio::co_spawn(get_executor(),
                              gdv->read_address(addr, result_buffer.span()),
                              boost::asio::use_future)
            .get(),
        std::exception);
}

BOOST_FIXTURE_TEST_CASE(valid_write_read_address, global_data_view_fixture) {

    auto gdv = get_data_view();
    const size_t block_size = 16;
    auto input_buffer = random_buffer(64 * block_size);

    address addr;
    addr.append(
        boost::asio::co_spawn(get_executor(),
                              gdv->write(input_buffer.string_view().substr(
                                             0 * block_size, 8 * block_size),
                                         {0}),
                              boost::asio::use_future)
            .get());
    addr.append(
        boost::asio::co_spawn(get_executor(),
                              gdv->write(input_buffer.string_view().substr(
                                             8 * block_size, 8 * block_size),
                                         {0}),
                              boost::asio::use_future)
            .get());
    addr.append(
        boost::asio::co_spawn(get_executor(),
                              gdv->write(input_buffer.string_view().substr(
                                             16 * block_size, 8 * block_size),
                                         {0}),
                              boost::asio::use_future)
            .get());
    addr.append(
        boost::asio::co_spawn(get_executor(),
                              gdv->write(input_buffer.string_view().substr(
                                             24 * block_size, 8 * block_size),
                                         {0}),
                              boost::asio::use_future)
            .get());
    addr.append(
        boost::asio::co_spawn(get_executor(),
                              gdv->write(input_buffer.string_view().substr(
                                             32 * block_size, 8 * block_size),
                                         {0}),
                              boost::asio::use_future)
            .get());
    addr.append(
        boost::asio::co_spawn(get_executor(),
                              gdv->write(input_buffer.string_view().substr(
                                             40 * block_size, 8 * block_size),
                                         {0}),
                              boost::asio::use_future)
            .get());
    addr.append(
        boost::asio::co_spawn(get_executor(),
                              gdv->write(input_buffer.string_view().substr(
                                             48 * block_size, 8 * block_size),
                                         {0}),
                              boost::asio::use_future)
            .get());
    addr.append(
        boost::asio::co_spawn(get_executor(),
                              gdv->write(input_buffer.string_view().substr(
                                             56 * block_size, 8 * block_size),
                                         {0}),
                              boost::asio::use_future)
            .get());

    BOOST_TEST(input_buffer.size() == addr.data_size());
    BOOST_TEST(addr.fragments.size() == 8);

    auto result_buffer = unique_buffer<char>(addr.data_size());
    boost::asio::co_spawn(get_executor(),
                          gdv->read_address(addr, result_buffer.span()),
                          boost::asio::use_future)
        .get();
    BOOST_TEST(input_buffer.string_view() == result_buffer.string_view());
}

BOOST_FIXTURE_TEST_CASE(write_link_unlink_free, global_data_view_fixture) {
    auto gdv = get_data_view();
    auto input_buffer = random_buffer(DEFAULT_PAGE_SIZE);

    std::cout << "start writing" << std::endl;
    auto addr = boost::asio::co_spawn(
                    get_executor(), gdv->write(input_buffer.string_view(), {0}),
                    boost::asio::use_future)
                    .get();
    BOOST_TEST(input_buffer.size() == addr.data_size());
    BOOST_TEST(addr.fragments.size() == 1ul);

    std::size_t used =
        boost::asio::co_spawn(get_executor(), gdv->get_used_space(),
                              boost::asio::use_future)
            .get();
    BOOST_TEST(used == input_buffer.size());

    boost::asio::co_spawn(get_executor(), gdv->link(addr),
                          boost::asio::use_future)
        .get();
    std::size_t freed = boost::asio::co_spawn(get_executor(), gdv->unlink(addr),
                                              boost::asio::use_future)
                            .get();
    BOOST_TEST(freed == 0ull);

    freed = boost::asio::co_spawn(get_executor(), gdv->unlink(addr),
                                  boost::asio::use_future)
                .get();
    BOOST_TEST(freed == input_buffer.size());

    used = boost::asio::co_spawn(get_executor(), gdv->get_used_space(),
                                 boost::asio::use_future)
               .get();
    BOOST_TEST(used == 0ull);

    // BOOST_CHECK_THROW(boost::asio::co_spawn(get_executor(),
    // gdv->unlink(addr),
    //                                         boost::asio::use_future)
    //                       .get(),
    //                   std::exception);
}

BOOST_FIXTURE_TEST_CASE(write_link_unlink_nofree, global_data_view_fixture) {
    auto gdv = get_data_view();
    auto input_buffer = random_buffer(DEFAULT_PAGE_SIZE / 2);

    std::cout << "start writing" << std::endl;
    auto addr = boost::asio::co_spawn(
                    get_executor(), gdv->write(input_buffer.string_view(), {0}),
                    boost::asio::use_future)
                    .get();
    BOOST_TEST(input_buffer.size() == addr.data_size());
    // BOOST_TEST(addr.fragments.size() == 1ul);

    std::size_t used =
        boost::asio::co_spawn(get_executor(), gdv->get_used_space(),
                              boost::asio::use_future)
            .get();
    BOOST_TEST(used == input_buffer.size());

    boost::asio::co_spawn(get_executor(), gdv->link(addr),
                          boost::asio::use_future)
        .get();
    std::size_t freed = boost::asio::co_spawn(get_executor(), gdv->unlink(addr),
                                              boost::asio::use_future)
                            .get();
    BOOST_TEST(freed == 0ull);

    freed = boost::asio::co_spawn(get_executor(), gdv->unlink(addr),
                                  boost::asio::use_future)
                .get();
    BOOST_TEST(freed == 0ull);

    used = boost::asio::co_spawn(get_executor(), gdv->get_used_space(),
                                 boost::asio::use_future)
               .get();
    BOOST_TEST(used == input_buffer.size());

    BOOST_CHECK_THROW(boost::asio::co_spawn(get_executor(), gdv->unlink(addr),
                                            boost::asio::use_future)
                          .get(),
                      std::exception);
}

BOOST_FIXTURE_TEST_CASE(write_offsets_unlink, global_data_view_fixture) {
    auto gdv = get_data_view();
    auto input_buffer = random_buffer(DEFAULT_PAGE_SIZE);
    std::vector<std::size_t> offsets = {0 * KIBI_BYTE, 2 * KIBI_BYTE,
                                        4 * KIBI_BYTE, 6 * KIBI_BYTE};

    auto addr =
        boost::asio::co_spawn(get_executor(),
                              gdv->write(input_buffer.string_view(), offsets),
                              boost::asio::use_future)
            .get();
    BOOST_TEST(input_buffer.size() == addr.data_size());
    BOOST_TEST(addr.fragments.size() == 1ul);
    std::size_t used =
        boost::asio::co_spawn(get_executor(), gdv->get_used_space(),
                              boost::asio::use_future)
            .get();
    auto frag = addr.get(0);
    BOOST_TEST(used == input_buffer.size());
    {
        address del_addr;
        del_addr.push({frag.pointer, 2 * KIBI_BYTE});
        std::size_t freed =
            boost::asio::co_spawn(get_executor(), gdv->unlink(del_addr),
                                  boost::asio::use_future)
                .get();
        BOOST_TEST(freed == 0);
    }
    {
        address del_addr;
        del_addr.push({frag.pointer + 2 * KIBI_BYTE, 2 * KIBI_BYTE});
        std::size_t freed =
            boost::asio::co_spawn(get_executor(), gdv->unlink(del_addr),
                                  boost::asio::use_future)
                .get();
        BOOST_TEST(freed == 0);
    }
    {
        address del_addr;
        del_addr.push({frag.pointer + 4 * KIBI_BYTE, 2 * KIBI_BYTE});
        std::size_t freed =
            boost::asio::co_spawn(get_executor(), gdv->unlink(del_addr),
                                  boost::asio::use_future)
                .get();
        BOOST_TEST(freed == 0);
    }
    {
        address del_addr;
        del_addr.push({frag.pointer + 6 * KIBI_BYTE, 2 * KIBI_BYTE});
        std::size_t freed =
            boost::asio::co_spawn(get_executor(), gdv->unlink(del_addr),
                                  boost::asio::use_future)
                .get();
        BOOST_TEST(freed == DEFAULT_PAGE_SIZE);
    }
}

} // namespace uh::cluster
