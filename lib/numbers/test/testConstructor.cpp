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

using namespace uh::numbers;
// ------------- Tests Follow --------------
BOOST_AUTO_TEST_CASE( init_from_fundamental_types )
{
    BigInteger a(23);
    BOOST_CHECK(a == 23);

    BigInteger b(-42);
    BOOST_CHECK(b == -42);

    BigInteger hex((std::size_t)0xFFFFFFFF);
    //BigInteger hex2("0xFFFFFFFF");
    BOOST_CHECK(hex == UINT32_MAX);
    //BOOST_CHECK(hex2 == UINT32_MAX);

    BigInteger bin((std::size_t)0b1001101);
    //BigInteger bin2("0b1001101");
    BOOST_CHECK(bin == 0b1001101);
    //BOOST_CHECK(bin2 == 0b1001101);

    /*
    BigInteger oct((std::size_t)8);
    BigInteger oct2("0o10");
    BigInteger oct3("10mod8");
    BOOST_CHECK(oct == 8);
    BOOST_CHECK(oct2 == 8);
    BOOST_CHECK(oct3 == 8);
    */

    BigInteger d((double)2.5);
    BOOST_CHECK(d == 2);

    //TODO: add test with long double

    BigInteger e((float)2.4);
    BOOST_CHECK(e == 2);

    BigInteger f((unsigned long)2);
    BOOST_CHECK(f == 2);
}

/*
BOOST_AUTO_TEST_CASE( init_from_string )
{
    std::string test_str="93326215443944152681699238856266700490715968264381621468592963895217599993229915608941463976156518286253697920827223758251185210916864000000000000000000000000";
    BigInteger c(test_str);

    BOOST_CHECK(c.as_string()==test_str);
}
 */

// ------------- Arithmetic Tests Follow --------------
BOOST_AUTO_TEST_CASE( plus_test )
{
    BigInteger a(std::numeric_limits<std::size_t>::max());
    BigInteger b(1);
    BigInteger c = a + b;
    BOOST_CHECK_MESSAGE(c.size()==2 and (c[0] == 0 and c[1] == 1), "Expecting 0xFFFFFFFFFFFFFFFF + 0x01 == 0x01|0000000000000000.");
    BOOST_CHECK_MESSAGE(a+BigInteger(0)==a, "Expecting that adding 0 to an arbitrary value does not change it.");
}

BOOST_AUTO_TEST_CASE( minus_test )
{
    BigInteger a(std::numeric_limits<std::size_t>::max());
    a++;
    //TODO: replace cumbersome initialization with string initialization to avoid testing addition here
    BigInteger b(1);
    BigInteger c = a - b;
    BOOST_CHECK_MESSAGE(c.size()==1 and c[0]==std::numeric_limits<std::size_t>::max(), "Expecting 0x01|0000000000000000 - 0x1 == 0xFFFFFFFFFFFFFFFF.");
    BOOST_CHECK_MESSAGE(a-BigInteger(0)==a, "Expecting that subtracting 0 from an arbitrary value does not change it.");
}

BOOST_AUTO_TEST_CASE( postfix_increment_test )
{
    BigInteger a(std::numeric_limits<std::size_t>::max());
    BigInteger b = a++;
    BOOST_CHECK_MESSAGE(b.size()==1 and (b[0] == std::numeric_limits<std::size_t>::max()), "For b = a++, b is expected to hold the pre-increment value of a (0xFFFFFFFFFFFFFFFF).");
    BOOST_CHECK_MESSAGE(a.size()==2 and (a[0] == 0 and a[1] == 1), "Expecting a == 0x01|0000000000000000 after performing postfix-increment.");
}

BOOST_AUTO_TEST_CASE( postfix_decrement_test )
{
    BigInteger a(std::numeric_limits<std::size_t>::max());
    a++;
    //TODO: replace cumbersome initialization with string initialization to avoid testing addition here
    BigInteger b = a--;
    BOOST_CHECK_MESSAGE(b.size()==2 and (b[0] == 0 and b[1] == 1), "For b = a--, b is expected to hold the pre-decrement value of a (0x01|0000000000000000).");
    BOOST_CHECK_MESSAGE(a.size()==1 and (a[0] == std::numeric_limits<std::size_t>::max()), "Expecting a == 0xFFFFFFFFFFFFFFFF after performing postfix-decrement.");
}

/*
BOOST_AUTO_TEST_CASE( mult_test )
{
    BigInteger a(std::numeric_limits<std::size_t>::max());
    BigInteger b(std::numeric_limits<std::size_t>::max());
    BigInteger c= a * b;
    BOOST_CHECK(c.size()==2);
    BOOST_CHECK(c==std::string("340282366920938463426481119284349108225"));
    BigInteger d = a * BigInteger(0);
    BOOST_CHECK(a*BigInteger(0)==BigInteger(0));
    BOOST_CHECK(a*BigInteger(1)==a);
}
*/
/*
BOOST_AUTO_TEST_CASE( divmod_test )
{
    BigInteger a(std::numeric_limits<std::size_t>::max());
    BigInteger b(std::numeric_limits<std::size_t>::max());
    BigInteger c = a * b;
    BOOST_CHECK(c.size()==2);
    BOOST_CHECK(c==std::string("340282366920938463426481119284349108225"));
    BOOST_CHECK(c/BigInteger(std::numeric_limits<std::size_t>::max())==BigInteger(std::numeric_limits<std::size_t>::max()));
    BOOST_CHECK(c%BigInteger(std::numeric_limits<std::size_t>::max())==BigInteger(0));
    c--;
    //BOOST_CHECK(c/std::numeric_limits<std::size_t>::max()==std::numeric_limits<std::size_t>::max()-1);
    //BOOST_CHECK(c%std::numeric_limits<std::size_t>::max()==std::numeric_limits<std::size_t>::max()-1);
}
*/

BOOST_AUTO_TEST_CASE( limb_array_is_zero_initialized)
{
    BigInteger zero(0);
    bool isZero = true;
    for(int i = 0; i < 8; ++i) {
        isZero = (zero[i] == 0) && isZero;
    }
    BOOST_CHECK_MESSAGE(isZero, "All element of the allocated array are supposed to be zero-initialized.");

}