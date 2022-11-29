//
// Created by benjamin-elias on 26.11.22.
//

#define BOOST_TEST_DYN_LINK
#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "uhLibConceptTypes variant filtering Tests"
#endif

#include <boost/test/unit_test.hpp>
#include <conceptTypes/conceptTypes.h>

BOOST_AUTO_TEST_CASE( vector_copy )
{
    BOOST_TEST_MESSAGE("Typename tester for concept VectorCopyArithmetic to detect AVX2 SIMD capable vectors: ");
    bool b = VectorCopyArithmetic<std::vector<int>, std::vector<int>>;
    BOOST_CHECK_EQUAL(b, 1);
    b = VectorCopyArithmetic<std::vector<int>, std::vector<double>>;
    BOOST_CHECK_EQUAL(b, 0);
    b = VectorCopyArithmetic<std::vector<int>, std::deque<int>>;
    BOOST_CHECK_EQUAL(b, 0);
    b = VectorCopyArithmetic<std::vector<int>, boost::container::vector<int>>;
    BOOST_CHECK_EQUAL(b, 1);
    b = VectorCopyArithmetic<std::vector<int>, boost::container::vector<double>>;
    BOOST_CHECK_EQUAL(b, 0);
    b = VectorCopyArithmetic<boost::container::vector<int>, std::deque<int>>;
    BOOST_CHECK_EQUAL(b, 0);
    b = VectorCopyArithmetic<boost::container::vector<boost::container::vector<int>>, boost::container::vector<boost::container::vector<int>>>;
    BOOST_CHECK_EQUAL(b, 0);
    std::cout << std::endl;
}

BOOST_AUTO_TEST_CASE( insert_copy )
{
    BOOST_TEST_MESSAGE("Typename tester for concept InsertCopy to detect fast container insert: ");
    bool b = InsertCopy<std::forward_list<int>, std::vector<int>>;
    BOOST_CHECK_EQUAL(b, 0);
    b = InsertCopy<std::vector<int>, std::vector<int>>;
    BOOST_CHECK_EQUAL(b, 1);
    b = InsertCopy<std::deque<int>, std::vector<int>>;
    BOOST_CHECK_EQUAL(b, 1);
    b = InsertCopy<std::deque<int>, std::vector<double>>;
    BOOST_CHECK_EQUAL(b, 0);
    std::cout << std::endl;
}

BOOST_AUTO_TEST_CASE( insert_copy_forward )
{
    BOOST_TEST_MESSAGE("Typename tester for concept InsertCopyForward to detect fast container insert: ");
    bool b = InsertCopyForward<std::forward_list<int>, std::vector<int>>;
    BOOST_CHECK_EQUAL(b, 1);
    b = InsertCopyForward<std::vector<int>, std::vector<int>>;
    BOOST_CHECK_EQUAL(b, 0);
    b = InsertCopyForward<std::deque<int>, std::vector<int>>;
    BOOST_CHECK_EQUAL(b, 0);
    b = InsertCopyForward<std::deque<int>, std::vector<double>>;
    BOOST_CHECK_EQUAL(b, 0);
    std::cout << std::endl;
}

BOOST_AUTO_TEST_CASE( unsigned_integral_convert_copy )
{
    BOOST_TEST_MESSAGE(
            "Typename tester for concept UnsignedIntegralConvertCopy to detect fast container conversion from unsigned integer rows: ");
    bool b = UnsignedIntegralConvertCopy<std::forward_list<unsigned int>, std::vector<unsigned short>>;
    BOOST_CHECK_EQUAL(b, 0);
    b = UnsignedIntegralConvertCopy<std::vector<unsigned int>, std::vector<unsigned int>>;
    BOOST_CHECK_EQUAL(b, 0);
    b = UnsignedIntegralConvertCopy<std::vector<unsigned char>, std::deque<unsigned long int>>;
    BOOST_CHECK_EQUAL(b, 1);
    b = UnsignedIntegralConvertCopy<std::vector<int>, std::deque<double>>;
    BOOST_CHECK_EQUAL(b, 0);
}

BOOST_AUTO_TEST_CASE( unsigned_integral_convert_copy_forward )
{
    BOOST_TEST_MESSAGE(
            "Typename tester for concept UnsignedIntegralConvertCopyForward to detect fast container conversion from unsigned integer rows: ");
    bool b = UnsignedIntegralConvertCopyForward<std::forward_list<unsigned int>, std::vector<unsigned short>>;
    BOOST_CHECK_EQUAL(b, 1);
    b = UnsignedIntegralConvertCopyForward<std::vector<unsigned int>, std::vector<unsigned int>>;
    BOOST_CHECK_EQUAL(b, 0);
    b = UnsignedIntegralConvertCopyForward<std::vector<unsigned char>, std::deque<unsigned long int>>;
    BOOST_CHECK_EQUAL(b, 0);
    b = UnsignedIntegralConvertCopyForward<std::vector<int>, std::deque<double>>;
    BOOST_CHECK_EQUAL(b, 0);
}

BOOST_AUTO_TEST_CASE( signed_integral_convert_copy ){
    BOOST_TEST_MESSAGE(
            "Typename tester for concept SignedIntegralConvertCopy to detect fast container conversion from unsigned integer rows: ");
    bool b = SignedIntegralConvertCopy<std::forward_list<signed int>, std::vector<signed short>>;
    BOOST_CHECK_EQUAL(b, 0);
    b = SignedIntegralConvertCopy<std::vector<signed int>, std::vector<signed int>>;
    BOOST_CHECK_EQUAL(b, 0);
    b = SignedIntegralConvertCopy<std::vector<signed char>, std::deque<signed long int>>;
    BOOST_CHECK_EQUAL(b, 1);
    b = SignedIntegralConvertCopy<std::vector<int>, std::deque<double>>;
    BOOST_CHECK_EQUAL(b, 0);
}

BOOST_AUTO_TEST_CASE( signed_integral_convert_copy_forward )
{
    BOOST_TEST_MESSAGE(
            "Typename tester for concept SignedIntegralConvertCopyForward to detect fast container conversion from unsigned integer rows: ");
    bool b = SignedIntegralConvertCopyForward<std::forward_list<signed int>, std::vector<signed short>>;
    BOOST_CHECK_EQUAL(b, 1);
    b = SignedIntegralConvertCopyForward<std::vector<signed int>, std::vector<signed int>>;
    BOOST_CHECK_EQUAL(b, 0);
    b = SignedIntegralConvertCopyForward<std::vector<signed char>, std::deque<signed long int>>;
    BOOST_CHECK_EQUAL(b, 0);
    b = SignedIntegralConvertCopyForward<std::vector<int>, std::deque<double>>;
    BOOST_CHECK_EQUAL(b, 0);
}

BOOST_AUTO_TEST_CASE( floating_point_convert_copy )
{
    BOOST_TEST_MESSAGE(
            "Typename tester for concept FloatingPointConvertCopy to detect fast container conversion into floating point rows: ");
    bool b = FloatingPointConvertCopy<std::forward_list<double>, std::vector<float>>;
    BOOST_CHECK_EQUAL(b, 0);
    b = FloatingPointConvertCopy<std::vector<double>, std::vector<double>>;
    BOOST_CHECK_EQUAL(b, 0);
    b = FloatingPointConvertCopy<std::deque<signed char>, std::vector<long double>>;
    BOOST_CHECK_EQUAL(b, 0);
    b = FloatingPointConvertCopy<std::deque<float>, std::vector<unsigned int>>;
    BOOST_CHECK_EQUAL(b, 1);
    b = FloatingPointConvertCopy<std::deque<long double>, std::vector<double>>;
    BOOST_CHECK_EQUAL(b, 1);
}

BOOST_AUTO_TEST_CASE( floating_point_convert_copy_forward )
{
    BOOST_TEST_MESSAGE(
            "Typename tester for concept FloatingPointConvertCopyForward to detect fast container conversion into floating point rows: ");
    bool b = FloatingPointConvertCopyForward<std::forward_list<double>, std::vector<float>>;
    BOOST_CHECK_EQUAL(b, 1);
    b = FloatingPointConvertCopyForward<std::vector<double>, std::vector<double>>;
    BOOST_CHECK_EQUAL(b, 0);
    b = FloatingPointConvertCopyForward<std::deque<signed char>, std::vector<long double>>;
    BOOST_CHECK_EQUAL(b, 0);
    b = FloatingPointConvertCopyForward<std::deque<float>, std::vector<unsigned int>>;
    BOOST_CHECK_EQUAL(b, 0);
    b = FloatingPointConvertCopyForward<std::deque<long double>, std::vector<double>>;
    BOOST_CHECK_EQUAL(b, 0);
}

BOOST_AUTO_TEST_CASE( round_convert_copy )
{
    BOOST_TEST_MESSAGE(
            "Typename tester for concept RoundConvertCopy to detect fast rounding container conversion from floating point rows: ");
    bool b = RoundConvertCopy<std::forward_list<int>, std::vector<float>>;
    BOOST_CHECK_EQUAL(b, 0);
    b = RoundConvertCopy<std::vector<double>, std::vector<double>>;
    BOOST_CHECK_EQUAL(b, 0);
    b = RoundConvertCopy<std::deque<signed char>, std::vector<long double>>;
    BOOST_CHECK_EQUAL(b, 1);
    b = RoundConvertCopy<std::deque<float>, std::vector<unsigned int>>;
    BOOST_CHECK_EQUAL(b, 0);
    b = RoundConvertCopy<std::deque<long double>, std::vector<double>>;
    BOOST_CHECK_EQUAL(b, 0);
}

BOOST_AUTO_TEST_CASE( round_convert_copy_forward )
{
    BOOST_TEST_MESSAGE(
            "Typename tester for concept RoundConvertCopyForward to detect fast rounding container conversion from floating point rows: ");
    bool b = RoundConvertCopyForward<std::forward_list<int>, std::vector<float>>;
    BOOST_CHECK_EQUAL(b, 1);
    b = RoundConvertCopyForward<std::vector<double>, std::vector<double>>;
    BOOST_CHECK_EQUAL(b, 0);
    b = RoundConvertCopyForward<std::deque<signed char>, std::vector<long double>>;
    BOOST_CHECK_EQUAL(b, 0);
    b = RoundConvertCopyForward<std::deque<float>, std::vector<unsigned int>>;
    BOOST_CHECK_EQUAL(b, 0);
    b = RoundConvertCopyForward<std::deque<long double>, std::vector<double>>;
    BOOST_CHECK_EQUAL(b, 0);
}

BOOST_AUTO_TEST_CASE( dynamic_cast_convert_copy )
{
    BOOST_TEST_MESSAGE("Typename tester for concept DynamicCastConvertCopy to convert lists of derived classes: ");
    struct Base {
        virtual ~Base() {}
    };

    struct Derived : Base {
        virtual void name() {}
    };
    bool b = DynamicCastConvertCopy<Base, Base>;
    BOOST_CHECK_EQUAL(b, 0);
    b = DynamicCastConvertCopy<Base, Derived>;
    BOOST_CHECK_EQUAL(b, 0);
    b = DynamicCastConvertCopy<Derived, Base>;
    BOOST_CHECK_EQUAL(b, 0);
    b = DynamicCastConvertCopy<Derived, Derived>;
    BOOST_CHECK_EQUAL(b, 0);
    b = DynamicCastConvertCopy<std::vector<Base>, std::vector<Base>>;
    BOOST_CHECK_EQUAL(b, 0);
    b = DynamicCastConvertCopy<std::forward_list<Base>, std::vector<Base>>;
    BOOST_CHECK_EQUAL(b, 0);
    b = DynamicCastConvertCopy<std::vector<Base>, std::vector<Derived>>;
    BOOST_CHECK_EQUAL(b, 1);
    b = DynamicCastConvertCopy<std::vector<Derived>, std::vector<Base>>;
    BOOST_CHECK_EQUAL(b, 0);
    b = DynamicCastConvertCopy<std::vector<Derived>, std::vector<Derived>>;
    BOOST_CHECK_EQUAL(b, 0);
    std::cout << std::endl;
}

BOOST_AUTO_TEST_CASE( dynamic_cast_convert_copy_forward )
{
    BOOST_TEST_MESSAGE(
            "Typename tester for concept DynamicCastConvertCopyForward to convert lists of derived classes: ");
    struct Base {
        virtual ~Base() {}
    };

    struct Derived : Base {
        virtual void name() {}
    };
    bool b = DynamicCastConvertCopyForward<Base, Base>;
    BOOST_CHECK_EQUAL(b, 0);
    b = DynamicCastConvertCopyForward<Base, Derived>;
    BOOST_CHECK_EQUAL(b, 0);
    b = DynamicCastConvertCopyForward<Derived, Base>;
    BOOST_CHECK_EQUAL(b, 0);
    b = DynamicCastConvertCopyForward<Derived, Derived>;
    BOOST_CHECK_EQUAL(b, 0);
    b = DynamicCastConvertCopyForward<std::vector<Base>, std::vector<Base>>;
    BOOST_CHECK_EQUAL(b, 0);
    b = DynamicCastConvertCopyForward<std::forward_list<Base>, std::vector<Base>>;
    BOOST_CHECK_EQUAL(b, 0);
    b = DynamicCastConvertCopyForward<std::forward_list<Derived>, std::vector<Base>>;
    BOOST_CHECK_EQUAL(b, 0);
    b = DynamicCastConvertCopyForward<std::forward_list<Base>, std::vector<Derived>>;
    BOOST_CHECK_EQUAL(b, 1);
    b = DynamicCastConvertCopyForward<std::vector<Base>, std::vector<Derived>>;
    BOOST_CHECK_EQUAL(b, 0);
    b = DynamicCastConvertCopyForward<std::vector<Derived>, std::vector<Base>>;
    BOOST_CHECK_EQUAL(b, 0);
    b = DynamicCastConvertCopyForward<std::vector<Derived>, std::vector<Derived>>;
    BOOST_CHECK_EQUAL(b, 0);
    std::cout << std::endl;
}