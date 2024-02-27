#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "big_int tests"
#endif

#include <boost/test/unit_test.hpp>
#include <common/types/big_int.h>

// ------------- Tests Suites Follow --------------

namespace uh::cluster {

constexpr const auto max_uint64 = std::numeric_limits<uint64_t>::max();

BOOST_AUTO_TEST_CASE(high_low) {
    BOOST_CHECK(big_int().get_high() == 0);
    BOOST_CHECK(big_int().get_low() == 0);

    BOOST_CHECK(big_int(1, 0).get_high() == 1);
    BOOST_CHECK(big_int(1, 0).get_low() == 0);
}

BOOST_AUTO_TEST_CASE(equality) {
    BOOST_CHECK(big_int() == big_int());
    BOOST_CHECK(big_int() == big_int(0));
    BOOST_CHECK(big_int() == 0);
    BOOST_CHECK(big_int() == big_int(0, 0));
    BOOST_CHECK(big_int(1) == big_int(0, 1));
    BOOST_CHECK(big_int(1) == 1);
}

BOOST_AUTO_TEST_CASE(greater_than) {
    BOOST_CHECK(big_int(1) > big_int());
    BOOST_CHECK(1 > big_int());
    BOOST_CHECK(big_int(1) > 0);
    BOOST_CHECK(!(big_int() > big_int()));
    BOOST_CHECK(!(big_int() > big_int(1)));

    BOOST_CHECK(big_int(1, 0) > big_int());
    BOOST_CHECK(big_int(2, 0) > big_int(1, 0));
    BOOST_CHECK(big_int(1, 0) > big_int(0, 1));

    BOOST_CHECK(big_int(2, 1) > big_int(1, 2));
}

BOOST_AUTO_TEST_CASE(greater_than_or_equal) {
    BOOST_CHECK(big_int(1) >= big_int());
    BOOST_CHECK(1 >= big_int());
    BOOST_CHECK(big_int(1) >= 0);
    BOOST_CHECK(big_int() >= big_int());
    BOOST_CHECK(!(big_int() >= big_int(1)));

    BOOST_CHECK(big_int(1, 0) >= big_int());
    BOOST_CHECK(big_int(2, 0) >= big_int(1, 0));
    BOOST_CHECK(big_int(1, 0) >= big_int(0, 1));
}

BOOST_AUTO_TEST_CASE(less_than) {
    BOOST_CHECK(big_int() < big_int(1));
    BOOST_CHECK(0 < big_int(1));
    BOOST_CHECK(big_int() < 1);
    BOOST_CHECK(!(big_int() < big_int()));
    BOOST_CHECK(!(big_int(1) < big_int()));

    BOOST_CHECK(big_int() < big_int(1, 0));
    BOOST_CHECK(big_int(1, 0) < big_int(2, 0));
    BOOST_CHECK(big_int(0, 1) < big_int(1, 0));
}

BOOST_AUTO_TEST_CASE(less_than_or_equal) {
    BOOST_CHECK(big_int() <= big_int(1));
    BOOST_CHECK(0 <= big_int(1));
    BOOST_CHECK(big_int() <= 1);
    BOOST_CHECK(big_int() <= big_int());
    BOOST_CHECK(!(big_int(1) <= big_int()));

    BOOST_CHECK(big_int() <= big_int(1, 0));
    BOOST_CHECK(big_int(1, 0) <= big_int(2, 0));
    BOOST_CHECK(big_int(0, 1) <= big_int(1, 0));
}

BOOST_AUTO_TEST_CASE(string) {
    BOOST_CHECK(big_int() == big_int(big_int().to_string()));
}

BOOST_AUTO_TEST_CASE(addition) {
    BOOST_CHECK(big_int() == big_int() + big_int());
    BOOST_CHECK(big_int() == big_int() + 0);
    BOOST_CHECK(big_int() == 0 + big_int());
    BOOST_CHECK(big_int(1) == big_int(1) + big_int());
    BOOST_CHECK(big_int(1) == big_int() + big_int(1));
    BOOST_CHECK(big_int(max_uint64) < big_int(max_uint64) + big_int(1));

    BOOST_CHECK(big_int(0, max_uint64) + big_int(0, 1) == big_int(1, 0));
    BOOST_CHECK(big_int(max_uint64) + big_int(1) == big_int(1, 0));
}

BOOST_AUTO_TEST_CASE(substraction) {
    BOOST_CHECK(big_int() == big_int() - big_int());
    BOOST_CHECK(big_int() == big_int() - 0);
    BOOST_CHECK(big_int(1) == big_int(1) - big_int());
    BOOST_CHECK(big_int(1) == 1 - big_int());
    BOOST_CHECK(big_int(max_uint64) > big_int(max_uint64) - big_int(1));

    BOOST_CHECK(big_int(max_uint64, max_uint64) == big_int() - big_int(1));
}

BOOST_AUTO_TEST_CASE(sub_and_add) {
    BOOST_CHECK((big_int() + big_int(1)) - big_int(1) == big_int());
    BOOST_CHECK((big_int() - big_int(1)) + big_int(1) == big_int());
}

BOOST_AUTO_TEST_CASE(multiplication) {
    BOOST_CHECK(big_int() * 0 == big_int());
    BOOST_CHECK(big_int(1) * 0 == big_int());
    BOOST_CHECK(big_int() * 1 == big_int());

    BOOST_CHECK(big_int(2) * 1 == big_int(2));
    BOOST_CHECK(big_int(1) * 2 == big_int(2));

    BOOST_CHECK(big_int(max_uint64) + big_int(max_uint64) ==
                big_int(2) * max_uint64);
    BOOST_CHECK(big_int(max_uint64) * 2 == big_int(2) * max_uint64);

    BOOST_CHECK(big_int(max_uint64) * 2 > big_int(max_uint64));
    BOOST_CHECK(big_int(max_uint64) < big_int(max_uint64) * 2);
}

BOOST_AUTO_TEST_CASE(mul_add) {
    BOOST_CHECK(big_int(2) * 3 == big_int(3) + big_int(3));
    BOOST_CHECK(big_int(2) * max_uint64 ==
                big_int(max_uint64) + big_int(max_uint64));
}

BOOST_AUTO_TEST_CASE(assign) {
    {
        big_int x(1, 1);
        BOOST_CHECK((x += x) == big_int(2, 2));
    }
    {
        big_int x(0, max_uint64);
        BOOST_CHECK((x += big_int(0, 1)) == big_int(1, 0));
    }
    {
        big_int x(0, max_uint64);
        BOOST_CHECK((x += big_int(0, 2)) == big_int(1, 1));
    }
}

BOOST_AUTO_TEST_CASE(data_store_big_int_test) {
    constexpr uint128_t b1{0, 16140901064495857661ul};
    constexpr uint64_t b2 = 13835058055282163710ul;
    constexpr big_int res1 = b1 * b2; // 223310303291865866574052609832259682310
    constexpr uint8_t answer1[] = {0xfc, 0xff, 0xff, 0xff, 0xff, 0xff,
                                   0xff, 0xa7, 0x06, 0x00, 0x00, 0x00,
                                   0x00, 0x00, 0x00, 0x00};
    BOOST_TEST(std::memcmp(res1.get_data(), answer1, sizeof(answer1)) == 0);

    constexpr uint128_t b3{0, 9213123140901423261ul};
    constexpr uint64_t b4 = 12332180552821123214ul;
    constexpr big_int res2 = b3 * b4; // 113617898028970796972859371597246680854
    constexpr uint8_t answer2[] = {0xe1, 0x7f, 0x0f, 0x37, 0xde, 0x02,
                                   0x7a, 0x55, 0x16, 0xcf, 0x22, 0xd3,
                                   0x35, 0xd3, 0x2d, 0xe9};
    BOOST_TEST(std::memcmp(res2.get_data(), answer2, sizeof(answer2)) == 0);

    constexpr auto res3 =
        res1 + res2; // 336928201320836663546911981429506363164
    constexpr uint8_t answer3[] = {0xdd, 0x7f, 0x0f, 0x37, 0xde, 0x02,
                                   0x7a, 0xfd, 0x1c, 0xcf, 0x22, 0xd3,
                                   0x35, 0xd3, 0x2d, 0xe9};
    BOOST_TEST(std::memcmp(res3.get_data(), answer3, sizeof(answer3)) == 0);

    constexpr auto res4 = res3 - res2;
    BOOST_TEST(std::memcmp(res4.get_data(), res1.get_data(), sizeof(res1)) ==
               0);

    BOOST_CHECK(res4 < res3);
    BOOST_CHECK(res3 > res2);
    BOOST_CHECK(res4 == res1);
}
} // namespace uh::cluster
