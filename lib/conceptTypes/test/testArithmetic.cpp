//
// Created by benjamin-elias on 26.11.22.
//

#define BOOST_TEST_DYN_LINK
#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "uhLibConceptTypes arithmetic type filtering Tests"
#endif

#include <boost/test/unit_test.hpp>
#include <conceptTypes/conceptTypes.h>

// ------------- Tests Follow --------------

BOOST_AUTO_TEST_CASE( integral_test )
{
    BOOST_TEST_MESSAGE("Typename tester for concept IntegralFundamental for any fundamental integral: ");
    BOOST_CHECK_EQUAL(IntegralFundamental<int>,1);
    BOOST_CHECK_EQUAL(IntegralFundamental<int&>,0);
    BOOST_CHECK_EQUAL(IntegralFundamental<float>,0);
    BOOST_CHECK_EQUAL(IntegralFundamental<float&>,0);
    BOOST_CHECK_EQUAL(IntegralFundamental<unsigned int>,1);
    BOOST_CHECK_EQUAL(IntegralFundamental<unsigned int&>,0);
    std::cout << std::endl;
    BOOST_TEST_MESSAGE("Typename tester for concept IntegralLValue for any lvalue integral: ");
    BOOST_CHECK_EQUAL(IntegralLValue<int>,0);
    BOOST_CHECK_EQUAL(IntegralLValue<int&>,1);
    BOOST_CHECK_EQUAL(IntegralLValue<float>,0);
    BOOST_CHECK_EQUAL(IntegralLValue<float&>,0);
    BOOST_CHECK_EQUAL(IntegralLValue<unsigned int>,0);
    BOOST_CHECK_EQUAL(IntegralLValue<unsigned int&>,1);
    std::cout << std::endl;
    BOOST_TEST_MESSAGE("Typename tester for concept IntegralPointer for any pointer integral: ");
    BOOST_CHECK_EQUAL(IntegralPointer<int>,0);
    BOOST_CHECK_EQUAL(IntegralPointer<int*>,1);
    BOOST_CHECK_EQUAL(IntegralPointer<float>,0);
    BOOST_CHECK_EQUAL(IntegralPointer<float*>,0);
    BOOST_CHECK_EQUAL(IntegralPointer<unsigned int>,0);
    BOOST_CHECK_EQUAL(IntegralPointer<unsigned int*>,1);
    std::cout << std::endl;
    BOOST_TEST_MESSAGE("Typename tester for concept Integral for any integral: ");
    BOOST_CHECK_EQUAL(Integral<int>,1);
    BOOST_CHECK_EQUAL(Integral<int&>,1);
    BOOST_CHECK_EQUAL(Integral<float>,0);
    BOOST_CHECK_EQUAL(Integral<float&>,0);
    BOOST_CHECK_EQUAL(Integral<unsigned int>,1);
    BOOST_CHECK_EQUAL(Integral<unsigned int&>,1);
    std::cout << std::endl;
}

BOOST_AUTO_TEST_CASE( unsigned_integral_test)
{
    BOOST_TEST_MESSAGE("Typename tester for concept UnsignedIntegralFundamental for any fundamental unsigned integral: ");
    BOOST_CHECK_EQUAL(UnsignedIntegralFundamental<int>,0);
    BOOST_CHECK_EQUAL(UnsignedIntegralFundamental<int&>,0);
    BOOST_CHECK_EQUAL(UnsignedIntegralFundamental<float>,0);
    BOOST_CHECK_EQUAL(UnsignedIntegralFundamental<float&>,0);
    BOOST_CHECK_EQUAL(UnsignedIntegralFundamental<unsigned int>,1);
    BOOST_CHECK_EQUAL(UnsignedIntegralFundamental<unsigned int&>,0);
    std::cout << std::endl;
    BOOST_TEST_MESSAGE("Typename tester for concept UnsignedIntegralLValue for any lvalue unsigned integral: ");
    BOOST_CHECK_EQUAL(UnsignedIntegralLValue<int>,0);
    BOOST_CHECK_EQUAL(UnsignedIntegralLValue<int&>,0);
    BOOST_CHECK_EQUAL(UnsignedIntegralLValue<float>,0);
    BOOST_CHECK_EQUAL(UnsignedIntegralLValue<float&>,0);
    BOOST_CHECK_EQUAL(UnsignedIntegralLValue<unsigned int>,0);
    BOOST_CHECK_EQUAL(UnsignedIntegralLValue<unsigned int&>,1);
    std::cout << std::endl;
    BOOST_TEST_MESSAGE("Typename tester for concept UnsignedIntegralPointer for any pointer unsigned integral: ");
    BOOST_CHECK_EQUAL(UnsignedIntegralPointer<int>,0);
    BOOST_CHECK_EQUAL(UnsignedIntegralPointer<int*>,0);
    BOOST_CHECK_EQUAL(UnsignedIntegralPointer<float>,0);
    BOOST_CHECK_EQUAL(UnsignedIntegralPointer<float*>,0);
    BOOST_CHECK_EQUAL(UnsignedIntegralPointer<unsigned int>,0);
    BOOST_CHECK_EQUAL(UnsignedIntegralPointer<unsigned int*>,1);
    std::cout << std::endl;
    BOOST_TEST_MESSAGE("Typename tester for concept UnsignedIntegral for any unsigned integral: ");
    BOOST_CHECK_EQUAL(UnsignedIntegral<int>,0);
    BOOST_CHECK_EQUAL(UnsignedIntegral<int&>,0);
    BOOST_CHECK_EQUAL(UnsignedIntegral<float>,0);
    BOOST_CHECK_EQUAL(UnsignedIntegral<float&>,0);
    BOOST_CHECK_EQUAL(UnsignedIntegral<unsigned int>,1);
    BOOST_CHECK_EQUAL(UnsignedIntegral<unsigned int&>,1);
    std::cout << std::endl;
}

BOOST_AUTO_TEST_CASE( signed_integral_test)
{
    BOOST_TEST_MESSAGE("Typename tester for concept SignedIntegralFundamental for any fundamental signed integral: ");
    BOOST_CHECK_EQUAL(SignedIntegralFundamental<int>,1);
    BOOST_CHECK_EQUAL(SignedIntegralFundamental<int&>,0);
    BOOST_CHECK_EQUAL(SignedIntegralFundamental<float>,0);
    BOOST_CHECK_EQUAL(SignedIntegralFundamental<float&>,0);
    BOOST_CHECK_EQUAL(SignedIntegralFundamental<unsigned int>,0);
    BOOST_CHECK_EQUAL(SignedIntegralFundamental<unsigned int&>,0);
    std::cout << std::endl;
    BOOST_TEST_MESSAGE("Typename tester for concept SignedIntegralLValue for any lvalue signed integral: ");
    BOOST_CHECK_EQUAL(SignedIntegralLValue<int>,0);
    BOOST_CHECK_EQUAL(SignedIntegralLValue<int&>,1);
    BOOST_CHECK_EQUAL(SignedIntegralLValue<float>,0);
    BOOST_CHECK_EQUAL(SignedIntegralLValue<float&>,0);
    BOOST_CHECK_EQUAL(SignedIntegralLValue<unsigned int>,0);
    BOOST_CHECK_EQUAL(SignedIntegralLValue<unsigned int&>,0);
    std::cout << std::endl;
    BOOST_TEST_MESSAGE("Typename tester for concept SignedIntegralPointer for any pointer signed integral: ");
    BOOST_CHECK_EQUAL(SignedIntegralPointer<int>,0);
    BOOST_CHECK_EQUAL(SignedIntegralPointer<int*>,1);
    BOOST_CHECK_EQUAL(SignedIntegralPointer<float>,0);
    BOOST_CHECK_EQUAL(SignedIntegralPointer<float*>,0);
    BOOST_CHECK_EQUAL(SignedIntegralPointer<unsigned int>,0);
    BOOST_CHECK_EQUAL(SignedIntegralPointer<unsigned int*>,0);
    std::cout << std::endl;
    BOOST_TEST_MESSAGE("Typename tester for concept SignedIntegral for any signed integral: ");
    BOOST_CHECK_EQUAL(SignedIntegral<int>,1);
    BOOST_CHECK_EQUAL(SignedIntegral<int&>,1);
    BOOST_CHECK_EQUAL(SignedIntegral<float>,0);
    BOOST_CHECK_EQUAL(SignedIntegral<float&>,0);
    BOOST_CHECK_EQUAL(SignedIntegral<unsigned int>,0);
    BOOST_CHECK_EQUAL(SignedIntegral<unsigned int&>,0);
    std::cout << std::endl;
}

BOOST_AUTO_TEST_CASE( floating_point_test )
{
    BOOST_TEST_MESSAGE("Typename tester for concept FloatingPointFundamental for any fundamental floating point: ");
    BOOST_CHECK_EQUAL(FloatingPointFundamental<int>,0);
    BOOST_CHECK_EQUAL(FloatingPointFundamental<int&>,0);
    BOOST_CHECK_EQUAL(FloatingPointFundamental<float>,1);
    BOOST_CHECK_EQUAL(FloatingPointFundamental<float&>,0);
    BOOST_CHECK_EQUAL(FloatingPointFundamental<unsigned int>,0);
    BOOST_CHECK_EQUAL(FloatingPointFundamental<unsigned int&>,0);
    std::cout << std::endl;
    BOOST_TEST_MESSAGE("Typename tester for concept FloatingPointLValue for any lvalue floating point: ");
    BOOST_CHECK_EQUAL(FloatingPointLValue<int>,0);
    BOOST_CHECK_EQUAL(FloatingPointLValue<int&>,0);
    BOOST_CHECK_EQUAL(FloatingPointLValue<float>,0);
    BOOST_CHECK_EQUAL(FloatingPointLValue<float&>,1);
    BOOST_CHECK_EQUAL(FloatingPointLValue<unsigned int>,0);
    BOOST_CHECK_EQUAL(FloatingPointLValue<unsigned int&>,0);
    std::cout << std::endl;
    BOOST_TEST_MESSAGE("Typename tester for concept FloatingPointPointer for any pointer floating point: ");
    BOOST_CHECK_EQUAL(FloatingPointPointer<int>,0);
    BOOST_CHECK_EQUAL(FloatingPointPointer<int*>,0);
    BOOST_CHECK_EQUAL(FloatingPointPointer<float>,0);
    BOOST_CHECK_EQUAL(FloatingPointPointer<float*>,1);
    BOOST_CHECK_EQUAL(FloatingPointPointer<unsigned int>,0);
    BOOST_CHECK_EQUAL(FloatingPointPointer<unsigned int*>,0);
    std::cout << std::endl;
    BOOST_TEST_MESSAGE("Typename tester for concept FloatingPoint for any floating point: ");
    BOOST_CHECK_EQUAL(FloatingPoint<int>,0);
    BOOST_CHECK_EQUAL(FloatingPoint<int&>,0);
    BOOST_CHECK_EQUAL(FloatingPoint<float>,1);
    BOOST_CHECK_EQUAL(FloatingPoint<float&>,1);
    BOOST_CHECK_EQUAL(FloatingPoint<unsigned int>,0);
    BOOST_CHECK_EQUAL(FloatingPoint<unsigned int&>,0);
    std::cout << std::endl;
}

BOOST_AUTO_TEST_CASE( arithmetic_test )
{
    BOOST_TEST_MESSAGE("Typename tester for concept ArithmeticFundamental for any fundamental arithmetic: ");
    BOOST_CHECK_EQUAL(ArithmeticFundamental<int>,1);
    BOOST_CHECK_EQUAL(ArithmeticFundamental<int&>,0);
    BOOST_CHECK_EQUAL(ArithmeticFundamental<float>,1);
    BOOST_CHECK_EQUAL(ArithmeticFundamental<float&>,0);
    BOOST_CHECK_EQUAL(ArithmeticFundamental<unsigned int>,1);
    BOOST_CHECK_EQUAL(ArithmeticFundamental<unsigned int&>,0);
    std::cout << std::endl;
    BOOST_TEST_MESSAGE("Typename tester for concept ArithmeticLValue for any lvalue arithmetic: ");
    BOOST_CHECK_EQUAL(ArithmeticLValue<int>,0);
    BOOST_CHECK_EQUAL(ArithmeticLValue<int&>,1);
    BOOST_CHECK_EQUAL(ArithmeticLValue<float>,0);
    BOOST_CHECK_EQUAL(ArithmeticLValue<float&>,1);
    BOOST_CHECK_EQUAL(ArithmeticLValue<unsigned int>,0);
    BOOST_CHECK_EQUAL(ArithmeticLValue<unsigned int&>,1);
    std::cout << std::endl;
    BOOST_TEST_MESSAGE("Typename tester for concept ArithmeticPointer for any pointer arithmetic: ");
    BOOST_CHECK_EQUAL(ArithmeticPointer<int>,0);
    BOOST_CHECK_EQUAL(ArithmeticPointer<int*>,1);
    BOOST_CHECK_EQUAL(ArithmeticPointer<float>,0);
    BOOST_CHECK_EQUAL(ArithmeticPointer<float*>,1);
    BOOST_CHECK_EQUAL(ArithmeticPointer<unsigned int>,0);
    BOOST_CHECK_EQUAL(ArithmeticPointer<unsigned int*>,1);
    std::cout << std::endl;
    BOOST_TEST_MESSAGE("Typename tester for concept Arithmetic for any arithmetic: ");
    BOOST_CHECK_EQUAL(Arithmetic<int>,1);
    BOOST_CHECK_EQUAL(Arithmetic<int&>,1);
    BOOST_CHECK_EQUAL(Arithmetic<float>,1);
    BOOST_CHECK_EQUAL(Arithmetic<float&>,1);
    BOOST_CHECK_EQUAL(Arithmetic<unsigned int>,1);
    BOOST_CHECK_EQUAL(Arithmetic<unsigned int&>,1);
    std::cout << std::endl;
}
