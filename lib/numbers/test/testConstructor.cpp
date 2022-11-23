//
// Created by max on 02.11.22.
//

#define BOOST_TEST_DYN_LINK
#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "Constructor Tests"
#endif

#include <boost/test/unit_test.hpp>
#include <numbers/BigInteger.h>

using namespace ultihash::numbers;
// ------------- Tests Follow --------------
BOOST_AUTO_TEST_CASE( init_from_fundamental_types )
{
    BigInteger a(23);
    BOOST_CHECK(a == 23);

    BigInteger b(-42);
    BOOST_CHECK(b == -42);

    BigInteger hex((std::size_t)0xFFFFFFFF);
    BigInteger hex2("0xFFFFFFFF");
    BOOST_CHECK(hex == UINT32_MAX);
    BOOST_CHECK(hex2 == UINT32_MAX);

    BigInteger bin((std::size_t)0b1001101);
    BigInteger bin2("0b1001101");
    BOOST_CHECK(bin == 0b1001101);
    BOOST_CHECK(bin2 == 0b1001101);

    BigInteger oct((std::size_t)8);
    BigInteger oct2("0o10");
    BigInteger oct3("10mod8");
    BOOST_CHECK(oct == 8);
    BOOST_CHECK(oct2 == 8);
    BOOST_CHECK(oct3 == 8);

    BigInteger d((long double)2.5);
    BOOST_CHECK(d == 3);

    BigInteger e((float)2.4);
    BOOST_CHECK(e == 2);

    BigInteger f((unsigned long)2);
    BOOST_CHECK(f == 2);
}

BOOST_AUTO_TEST_CASE( init_from_string )
{
    std::string test_str="93326215443944152681699238856266700490715968264381621468592963895217599993229915608941463976156518286253697920827223758251185210916864000000000000000000000000";
    BigInteger c(test_str);

    BOOST_CHECK(c.as_string()==test_str);
}

//TODO: while this test probably belongs somewhere else, it will remain here for the time being
BOOST_AUTO_TEST_CASE( arrays_are_zero_initialized)
{
    std::unique_ptr<std::size_t[]> outPtr(new (std::align_val_t(64)) std::size_t[512]{});
    bool isZero = true;
    for(int i = 0; i < 512; i++) {
        isZero = (outPtr[i] == 0) && isZero;
    }
    BOOST_CHECK_MESSAGE(isZero, "All element of the allocated array are supposed to be zero-initialized.");
}

