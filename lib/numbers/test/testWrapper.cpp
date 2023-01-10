//
// Created by max on 02.11.22.
//

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

using namespace uh::numbers;
namespace bdata = boost::unit_test::data;

// ------------- Tests Follow --------------
BOOST_AUTO_TEST_CASE( dummy )
{
    BOOST_CHECK(true);
}
/*
BOOST_DATA_TEST_CASE(wrapper_correctnes, (bdata::random(0,2147483647) ^ bdata::xrange(5)) * (bdata::random(0,2147483647) ^ bdata::xrange(5)), sample1, id1, sample2, id2){
    BigInteger bigIntA(sample1);
    BigInteger bigIntB(sample2);
    mpz_class gmpA = sample1;
    mpz_class gmpB = sample2;

    BOOST_CHECK((+bigIntA).getNumber() == +gmpA);
    BOOST_CHECK((+bigIntB).getNumber() == +gmpB);
    BOOST_CHECK((-bigIntA).getNumber() == -gmpA);
    BOOST_CHECK((-bigIntB).getNumber() == -gmpB);

    BOOST_CHECK((bigIntA + bigIntB).getNumber() == (gmpA + gmpB));
    BOOST_CHECK((bigIntA * bigIntB).getNumber() == (gmpA * gmpB));

    if(sample1 >= sample2) {
        BOOST_CHECK((bigIntA - bigIntB).getNumber() == (gmpA - gmpB));
        BOOST_CHECK((bigIntA / bigIntB).getNumber() == (gmpA / gmpB));
        BOOST_CHECK((bigIntA % bigIntB).getNumber() == (gmpA % gmpB));
    } else {
        BOOST_CHECK((bigIntB - bigIntA).getNumber() == (gmpB - gmpA));
        BOOST_CHECK((bigIntB / bigIntA).getNumber() == (gmpB / gmpA));
        BOOST_CHECK((bigIntB % bigIntA).getNumber() == (gmpB % gmpA));
    }
    BOOST_CHECK((~bigIntA).getNumber() == ~gmpA);
    BOOST_CHECK((~bigIntB).getNumber() == ~gmpB);

    BOOST_CHECK((bigIntA & bigIntB).getNumber() == (gmpA & gmpB));
    BOOST_CHECK((bigIntA | bigIntB).getNumber() == (gmpA | gmpB));
    BOOST_CHECK((bigIntA ^ bigIntB).getNumber() == (gmpA ^ gmpB));

    BOOST_CHECK((bigIntA << sample2).getNumber() == (gmpA << sample2));
    BOOST_CHECK((bigIntA >> sample2).getNumber() == (gmpA >> sample2));
    */
    /*
    BigInteger bigIntA(sample1);
    BigInteger bigIntB(sample2);
    BigInteger bigIntC(0);
    mpz_class gmpA = sample1;
    mpz_class gmpB = sample2;

    mpz_class gmpC=+gmpA;
    BOOST_CHECK((+bigIntA).getNumber() == +gmpC);
    gmpC=+gmpB;
    BOOST_CHECK((+bigIntB).getNumber() == +gmpC);
    gmpC=-gmpA;
    BOOST_CHECK((-bigIntA).getNumber() == (-gmpC));
    gmpC=-gmpB;
    BOOST_CHECK((-bigIntB).as_string() == (-gmpC));

    gmpC=gmpA+gmpB;
    BOOST_CHECK((bigIntA + bigIntB).getNumber() == gmpC);
    gmpC=gmpA*gmpB;
    BOOST_CHECK((bigIntA * bigIntB).getNumber() == gmpC);

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
    */
//}
