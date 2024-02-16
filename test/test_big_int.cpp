#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "big_int tests"
#endif

#include <boost/test/unit_test.hpp>
#include <common/types/big_int.h>
#include <iostream>

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
    // compilation fails: BOOST_CHECK(big_int() == 0 + big_int());
    BOOST_CHECK(big_int(1) == big_int(1) + big_int());
    BOOST_CHECK(big_int(1) == big_int() + big_int(1));
    BOOST_CHECK(big_int(max_uint64) < big_int(max_uint64) + big_int(1));

    // fails: BOOST_CHECK(big_int(0, std::numeric_limits<uint64_t>::max()) +
    // big_int(0, 1) == big_int(1, 0)); fails:
    // BOOST_CHECK(big_int(std::numeric_limits<uint64_t>::max()) + big_int(1) ==
    // big_int(1, 0));
}

BOOST_AUTO_TEST_CASE(substraction) {
    BOOST_CHECK(big_int() == big_int() - big_int());
    BOOST_CHECK(big_int() == big_int() - 0);
    BOOST_CHECK(big_int(1) == big_int(1) - big_int());
    // compilation fails: BOOST_CHECK(big_int(1) == 1 - big_int());
    BOOST_CHECK(big_int(max_uint64) > big_int(max_uint64) - big_int(1));

    // why this format?
    BOOST_CHECK(big_int(max_uint64, max_uint64 - 1) == big_int() - big_int(1));
}

BOOST_AUTO_TEST_CASE(sub_and_add) {
    BOOST_CHECK((big_int() + big_int(1)) - big_int(1) == big_int());

    // fails: BOOST_CHECK((big_int() - big_int(1)) + big_int(1) == big_int());
}

BOOST_AUTO_TEST_CASE(multiplication) {
    BOOST_CHECK(big_int() * 0 == big_int());
    BOOST_CHECK(big_int(1) * 0 == big_int());
    BOOST_CHECK(big_int() * 1 == big_int());

    BOOST_CHECK(big_int(2) * 1 == big_int(2));
    BOOST_CHECK(big_int(1) * 2 == big_int(2));

    // fails: BOOST_CHECK(big_int(max_uint64) + big_int(max_uint64) ==
    // big_int(2) * max_uint64);
    BOOST_CHECK(big_int(max_uint64) * 2 == big_int(2) * max_uint64);

    BOOST_CHECK(big_int(max_uint64) * 2 > big_int(max_uint64));
    BOOST_CHECK(big_int(max_uint64) < big_int(max_uint64) * 2);
}

BOOST_AUTO_TEST_CASE(mul_add) {
    BOOST_CHECK(big_int(2) * 3 == big_int(3) + big_int(3));
    // fails: BOOST_CHECK(big_int(2) * max_uint64 == big_int(max_uint64) +
    // big_int(max_uint64));
}

BOOST_AUTO_TEST_CASE(assign) {
    {
        big_int x(1, 1);
        BOOST_CHECK((x += x) == big_int(2, 2));
    }
    /* fails:
    {
        big_int x(0, max_uint64);
        BOOST_CHECK(x += big_int(0, 1) == big_int(1, 0));
    }*/
}

} // namespace uh::cluster
