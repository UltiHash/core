//
// Created by max on 02.11.22.
//

#define BOOST_TEST_DYN_LINK
#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "Wrapper Tests"
#endif

#include <boost/test/unit_test.hpp>
#include <boost/test/data/test_case.hpp>
#include <boost/test/data/monomorphic.hpp>
#include <numbers/BigInteger.h>
#include <gmpxx.h>

using namespace ultihash::numbers;
namespace bdata = boost::unit_test::data;

// ------------- Tests Follow --------------
BOOST_DATA_TEST_CASE(wrapper_correctnes, (bdata::random(0,2147483647) ^ bdata::xrange(5)) * (bdata::random(0,2147483647) ^ bdata::xrange(5)), sample1, id1, sample2, id2){
    BigInteger bigIntA(sample1);
    BigInteger bigIntB(sample2);
    mpz_class gmpA = sample1;
    mpz_class gmpB = sample2;

    mpz_class gmpC=+gmpA;
    BOOST_CHECK((+bigIntA).as_string() == gmpC.get_str());
    gmpC=+gmpB;
    BOOST_CHECK((+bigIntB).as_string() == gmpC.get_str());
    gmpC=-gmpA;
    BOOST_CHECK((-bigIntA).as_string() == gmpC.get_str());
    gmpC=-gmpB;
    BOOST_CHECK((-bigIntB).as_string() == gmpC.get_str());

    gmpC=gmpA+gmpB;
    BOOST_CHECK((bigIntA + bigIntB).as_string() == gmpC.get_str());
    gmpC=gmpA*gmpB;
    BOOST_CHECK((bigIntA * bigIntB).as_string() == gmpC.get_str());

    if(sample1 >= sample2) {
        gmpC=gmpA-gmpB;
        BOOST_CHECK((bigIntA - bigIntB).as_string() == gmpC.get_str());
        gmpC=gmpA/gmpB;
        BOOST_CHECK((bigIntA / bigIntB).as_string() == gmpC.get_str());
        gmpC=gmpA%gmpB;
        BOOST_CHECK((bigIntA % bigIntB).as_string() == gmpC.get_str());
    } else {
        gmpC=gmpB-gmpA;
        BOOST_CHECK((bigIntB - bigIntA).as_string() == gmpC.get_str());
        gmpC=gmpB/gmpA;
        BOOST_CHECK((bigIntB / bigIntA).as_string() == gmpC.get_str());
        gmpC=gmpB%gmpA;
        BOOST_CHECK((bigIntB % bigIntA).as_string() == gmpC.get_str());
    }
    gmpC=~gmpA;
    BOOST_CHECK((~bigIntA).as_string() == gmpC.get_str());
    gmpC=~gmpB;
    BOOST_CHECK((~bigIntB).as_string() == gmpC.get_str());

    gmpC=gmpA&gmpB;
    BOOST_CHECK((bigIntA & bigIntB).as_string() == gmpC.get_str());
    gmpC=gmpA|gmpB;
    BOOST_CHECK((bigIntA | bigIntB).as_string() == gmpC.get_str());
    gmpC=gmpA^gmpB;
    BOOST_CHECK((bigIntA ^ bigIntB).as_string() == gmpC.get_str());

    gmpC=gmpA<<1;
    BOOST_CHECK((bigIntA << 1).as_string() == gmpC.get_str());
    gmpC=gmpB>>1;
    BOOST_CHECK((bigIntA >> 1).as_string() == gmpC.get_str());

    BOOST_CHECK((sample1 < sample2 and bigIntA < bigIntB) xor (sample2 < sample1 and bigIntB < bigIntA));
    BOOST_CHECK((sample1 > sample2 and bigIntA > bigIntB) xor (sample2 > sample1 and bigIntB > bigIntA));
    BOOST_CHECK((sample1 <= sample2 and bigIntA <= bigIntB) or (sample2 <= sample1 and bigIntB <= bigIntA));
    BOOST_CHECK((sample1 >= sample2 and bigIntA >= bigIntB) or (sample2 >= sample1 and bigIntB >= bigIntA));
    BOOST_CHECK((sample1 == sample2 and bigIntA == bigIntB) or (sample1 != sample2 and bigIntA != bigIntB));
}
