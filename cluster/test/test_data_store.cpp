//
// Created by masi on 7/24/23.
//
#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "data_store tests"
#endif

#include <boost/test/unit_test.hpp>
#include "common.h"
#include "free_spot_manager.h"


// ------------- Tests Suites Follow --------------


namespace uh::cluster {

// ---------------------------------------------------------------------

struct fs_fixture
{

};

// ---------------------------------------------------------------------

BOOST_AUTO_TEST_CASE (test_big_int)
{
    constexpr uint128_t b1 {0, 16140901064495857661ul};
    constexpr uint64_t b2 = 13835058055282163710ul;
    constexpr big_int res1 = b1 * b2; // 223310303291865866574052609832259682310
    constexpr uint8_t answer1 [] = {0xfc, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xa7,
                         0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    BOOST_TEST(std::memcmp (res1.get_data(), answer1, sizeof(answer1)) == 0);

    constexpr uint128_t b3 {0, 9213123140901423261ul};
    constexpr uint64_t b4 = 12332180552821123214ul;
    constexpr big_int res2 = b3 * b4; // 113617898028970796972859371597246680854
    constexpr uint8_t answer2 [] = {0xe1, 0x7f, 0x0f, 0x37, 0xde, 0x02, 0x7a, 0x55,
                                    0x16, 0xcf, 0x22, 0xd3, 0x35, 0xd3, 0x2d, 0xe9};
    BOOST_TEST(std::memcmp (res2.get_data(), answer2, sizeof(answer2)) == 0);

    constexpr auto res3 = res1 + res2; // 336928201320836663546911981429506363164
    constexpr uint8_t answer3 [] = {0xdd, 0x7f, 0x0f, 0x37, 0xde, 0x02, 0x7a, 0xfd,
                                    0x1c, 0xcf, 0x22, 0xd3, 0x35, 0xd3, 0x2d, 0xe9};
    BOOST_TEST(std::memcmp (res3.get_data(), answer3, sizeof(answer3)) == 0);


    constexpr auto res4 = res3 - res2;
    BOOST_TEST(std::memcmp (res4.get_data(), res1.get_data(), sizeof(res1)) == 0);

    BOOST_CHECK(res4 < res3);
    BOOST_CHECK(res3 > res2);
    BOOST_CHECK(res4 == res1);

}
// ---------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE (test_free_spot_manager, fs_fixture)
{
    std::filesystem::remove ("__test_free_spot_manager");
    std::array <uint128_t, 100> offsets;
    std::array <std::size_t , 100> sizes {};

    for (auto& offset: offsets) {
        const auto n0 = rand () % std::numeric_limits <uint64_t>::max();
        const auto n1 = rand () % std::numeric_limits <uint64_t>::max();
        offset = {n0, n1};
    }

    for (auto& size: sizes) {
        size = {rand () % std::numeric_limits <uint64_t>::max()};
    }

    std::size_t expected_total_free_size = 0;
    {
        free_spot_manager fsm("__test_free_spot_manager");
        const auto free_spot_size1 = fsm.total_free_spots();
        BOOST_CHECK((free_spot_size1 == big_int {0, 0}));

        const auto fs1 = fsm.pop_free_spot();
        BOOST_CHECK(fs1 == std::nullopt);
        fsm.apply_popped_items();
        const auto free_spot_size2 = fsm.total_free_spots();

        BOOST_CHECK((free_spot_size2 == big_int {0, 0}));
        const auto fs2 = fsm.pop_free_spot();
        BOOST_CHECK(fs2 == std::nullopt);


        for (int i = 0; i < offsets.size(); ++i) {
            fsm.push_free_spot(offsets[i], sizes[i]);
            expected_total_free_size += sizes[i];

            const auto free_spot_size3 = fsm.total_free_spots();
            BOOST_CHECK((free_spot_size3 == big_int {0, expected_total_free_size}));
        }

        for (int i = 0; i < 20; ++i) {
            const auto fs = fsm.pop_free_spot();
            BOOST_CHECK (fs.value().pointer == offsets.at(i));
            BOOST_CHECK (fs.value().size == sizes.at(i));
            expected_total_free_size -= fs.value().size;

        }
        fsm.apply_popped_items();
        for (int i = 20; i < 40; ++i) {
            const auto fs = fsm.pop_free_spot();
            BOOST_CHECK (fs.value().pointer == offsets.at(i));
            BOOST_CHECK (fs.value().size == sizes.at(i));
        }

        const auto free_spot_size3 = fsm.total_free_spots();
        BOOST_CHECK((free_spot_size3 == big_int {0, expected_total_free_size}));

    }

    {

        free_spot_manager fsm("__test_free_spot_manager");
        const auto free_spot_size1 = fsm.total_free_spots();
        BOOST_CHECK((free_spot_size1 == big_int {0, expected_total_free_size}));

        for (int i = 20; i < 40; ++i) {
            const auto fs = fsm.pop_free_spot();
            BOOST_CHECK (fs.value().pointer == offsets.at(i));
            BOOST_CHECK (fs.value().size == sizes.at(i));
            expected_total_free_size -= fs.value().size;

        }

        fsm.apply_popped_items();

        const auto free_spot_size2 = fsm.total_free_spots();
        BOOST_CHECK((free_spot_size2 == big_int {0, expected_total_free_size}));

        for (int i = 0; i < 20; ++i) {
            fsm.push_free_spot(offsets[i], sizes[i]);
            expected_total_free_size += sizes[i];

            const auto free_spot_size3 = fsm.total_free_spots();
            BOOST_CHECK((free_spot_size3 == big_int {0, expected_total_free_size}));
        }

        for (int i = 40; i < 60; ++i) {
            const auto fs = fsm.pop_free_spot();
            BOOST_CHECK (fs.value().pointer == offsets.at(i));
            BOOST_CHECK (fs.value().size == sizes.at(i));
            expected_total_free_size -= fs.value().size;

        }
        fsm.apply_popped_items();

        const auto free_spot_size4 = fsm.total_free_spots();
        BOOST_CHECK((free_spot_size4 == big_int {0, expected_total_free_size}));

        for (int i = 60; i < 100; ++i) {
            const auto fs = fsm.pop_free_spot();
            BOOST_CHECK (fs.value().pointer == offsets.at(i));
            BOOST_CHECK (fs.value().size == sizes.at(i));
            expected_total_free_size -= fs.value().size;

        }
        for (int i = 0; i < 20; ++i) {
            const auto fs = fsm.pop_free_spot();
            BOOST_CHECK (fs.value().pointer == offsets.at(i));
            BOOST_CHECK (fs.value().size == sizes.at(i));
            expected_total_free_size -= fs.value().size;

        }
        fsm.apply_popped_items();
        const auto free_spot_size5 = fsm.total_free_spots();
        BOOST_CHECK((free_spot_size5 == big_int {0, 0}));

        const auto fs1 = fsm.pop_free_spot();
        BOOST_CHECK(fs1 == std::nullopt);

        for (int i = 0; i < 20; ++i) {
            fsm.push_free_spot(offsets[i], sizes[i]);
            expected_total_free_size += sizes[i];

            const auto free_spot_size3 = fsm.total_free_spots();
            BOOST_CHECK((free_spot_size3 == big_int {0, expected_total_free_size}));
        }

        for (int i = 0; i < 20; ++i) {
            const auto fs = fsm.pop_free_spot();
            BOOST_CHECK (fs.value().pointer == offsets.at(i));
            BOOST_CHECK (fs.value().size == sizes.at(i));
            expected_total_free_size -= fs.value().size;

        }
        fsm.apply_popped_items();
        const auto free_spot_size6 = fsm.total_free_spots();
        BOOST_CHECK((free_spot_size6 == big_int {0, 0}));
        const auto free_spot_size7 = fsm.total_free_spots();
        BOOST_CHECK((free_spot_size7 == big_int {0, 0}));

        const auto fs2 = fsm.pop_free_spot();
        BOOST_CHECK(fs2 == std::nullopt);
    }

    std::filesystem::remove ("__test_free_spot_manager");
}

// ---------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE (test_data_store, fs_fixture)
{

}
// ---------------------------------------------------------------------

} // end namespace uh::cluster