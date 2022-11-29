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

BOOST_AUTO_TEST_CASE( variant_test )
{
    BOOST_TEST_MESSAGE("Typename tester for concept VaraintSingleRecursion to detect std::variant: ");
    BOOST_CHECK_EQUAL(VariantSingleRecursion<std::variant<>>,0);
    BOOST_CHECK_EQUAL(VariantSingleRecursion<int>,0);
    BOOST_CHECK_EQUAL(VariantSingleRecursion<std::vector<int>>,0);
    bool b=VariantSingleRecursion<std::variant<int,double>>;
    BOOST_CHECK_EQUAL(b,1);
    b=VariantSingleRecursion<std::variant<std::vector<int>,std::deque<int>>>;
    BOOST_CHECK_EQUAL(b,1);
    b=VariantSingleRecursion<std::variant<int,std::variant<unsigned int,float>>>;
    BOOST_CHECK_EQUAL(b,0);
    b=VariantSingleRecursion<std::variant<std::variant<unsigned int,float>,std::variant<unsigned int,float>>>;
    BOOST_CHECK_EQUAL(b,0);
    b=VariantSingleRecursion<std::variant<std::variant<unsigned int,float>,std::vector<int>>>;
    BOOST_CHECK_EQUAL(b,0);
    std::cout << std::endl;

    BOOST_TEST_MESSAGE("Typename tester for concept DeepVariantContainer to detect conform aiHeuristics or std::variant: ");
    BOOST_CHECK_EQUAL(DeepVariantContainer<std::variant<>>,0);
    BOOST_CHECK_EQUAL(DeepVariantContainer<int>,0);
    BOOST_CHECK_EQUAL(DeepVariantContainer<std::vector<int>>,0);
    b=DeepVariantContainer<std::variant<int,double>>;
    BOOST_CHECK_EQUAL(b,1);
    b=DeepVariantContainer<std::variant<std::vector<int>,std::deque<int>>>;
    BOOST_CHECK_EQUAL(b,1);
    b=DeepVariantContainer<std::variant<int,std::variant<unsigned int,float>>>;
    BOOST_CHECK_EQUAL(b,0);
    b=DeepVariantContainer<std::variant<std::variant<unsigned int,float>,std::variant<unsigned int,float>>>;
    BOOST_CHECK_EQUAL(b,1);
    b=DeepVariantContainer<std::variant<std::variant<unsigned int,float>,std::vector<int>>>;
    BOOST_CHECK_EQUAL(b,1);
    std::cout << std::endl;

    BOOST_TEST_MESSAGE("Typename tester for concept OptionalRecursiveContainer to detect conform aiHeuristics or std::variant or arithmetic: ");
    b=OptionalRecursiveContainer<std::variant<>>;
    BOOST_CHECK_EQUAL(b,0);
    b=OptionalRecursiveContainer<int>;
    BOOST_CHECK_EQUAL(b,0);
    b=OptionalRecursiveContainer<int&>;
    BOOST_CHECK_EQUAL(b,0);
    b=OptionalRecursiveContainer<std::vector<int>>;
    BOOST_CHECK_EQUAL(b,1);
    b=OptionalRecursiveContainer<std::variant<int,double>>;
    BOOST_CHECK_EQUAL(b,1);
    b=OptionalRecursiveContainer<std::variant<std::vector<int>,std::deque<int>>>;
    BOOST_CHECK_EQUAL(b,1);
    b=OptionalRecursiveContainer<std::variant<int,std::variant<unsigned int,float>>>;
    BOOST_CHECK_EQUAL(b,0);
    b=OptionalRecursiveContainer<std::variant<std::variant<unsigned int,float>,std::variant<unsigned int,float>>>;
    BOOST_CHECK_EQUAL(b,1);
    b=OptionalRecursiveContainer<std::variant<std::variant<unsigned int,float>,std::vector<int>>>;
    BOOST_CHECK_EQUAL(b,1);
    std::cout << std::endl;

    BOOST_TEST_MESSAGE("Typename tester for concept OptionalRecursiveArithmeticContainer to detect conform aiHeuristics or std::variant or arithmetic: ");
    b=OptionalRecursiveArithmeticContainer<std::variant<>>;
    BOOST_CHECK_EQUAL(b,0);
    b=OptionalRecursiveArithmeticContainer<int>;
    BOOST_CHECK_EQUAL(b,1);
    b=OptionalRecursiveArithmeticContainer<int&>;
    BOOST_CHECK_EQUAL(b,1);
    b=OptionalRecursiveArithmeticContainer<std::vector<int>>;
    BOOST_CHECK_EQUAL(b,1);
    b=OptionalRecursiveArithmeticContainer<std::variant<int,double>>;
    BOOST_CHECK_EQUAL(b,1);
    b=OptionalRecursiveArithmeticContainer<std::variant<std::vector<int>,std::deque<int>>>;
    BOOST_CHECK_EQUAL(b,1);
    b=OptionalRecursiveArithmeticContainer<std::variant<int,std::variant<unsigned int,float>>>;
    BOOST_CHECK_EQUAL(b,0);
    b=OptionalRecursiveArithmeticContainer<std::variant<std::variant<unsigned int,float>,std::variant<unsigned int,float>>>;
    BOOST_CHECK_EQUAL(b,1);
    b=OptionalRecursiveArithmeticContainer<std::variant<std::variant<unsigned int,float>,std::vector<int>>>;
    BOOST_CHECK_EQUAL(b,1);
    std::cout << std::endl;
}

BOOST_AUTO_TEST_CASE( inner_container_type_of_test )
{
    BOOST_TEST_MESSAGE("Typename tester for concept InnerContainerTypeOf to detect inner container types: ");
    bool b=InnerContainerTypeOf<int,std::vector<int>>;
    BOOST_CHECK_EQUAL(b,1);
    b=InnerContainerTypeOf<int,int>;
    BOOST_CHECK_EQUAL(b,0);
    b=InnerContainerTypeOf<std::vector<int>,std::vector<int>>;
    BOOST_CHECK_EQUAL(b,0);
    b=InnerContainerTypeOf<std::variant<std::vector<int>,is_deque<int>>,std::vector<std::variant<std::vector<int>,is_deque<int>>>>;
    BOOST_CHECK_EQUAL(b,1);
    std::cout << std::endl;

    BOOST_TEST_MESSAGE("Typename tester for concept InnerContainerTypeOf references: ");
    b=InnerContainerTypeOf<int&,std::vector<int&>>;
    BOOST_CHECK_EQUAL(b,1);
    b=InnerContainerTypeOf<int&,int&>;
    BOOST_CHECK_EQUAL(b,0);
    b=InnerContainerTypeOf<std::vector<int&>,std::vector<int&>>;
    BOOST_CHECK_EQUAL(b,0);
    b=InnerContainerTypeOf<std::variant<std::vector<int&>,is_deque<int&>>,std::vector<std::variant<std::vector<int&>,is_deque<int&>>>>;
    BOOST_CHECK_EQUAL(b,1);
    std::cout << std::endl;

    b=InnerContainerTypeOf<int&&,std::vector<int&&>>;
    BOOST_CHECK_EQUAL(b,1);
    b=InnerContainerTypeOf<int&&,int&&>;
    BOOST_CHECK_EQUAL(b,0);
    b=InnerContainerTypeOf<std::vector<int&&>,std::vector<int&&>>;
    BOOST_CHECK_EQUAL(b,0);
    b=InnerContainerTypeOf<std::variant<std::vector<int&&>,is_deque<int&&>>,std::vector<std::variant<std::vector<int&&>,is_deque<int&&>>>>;
    BOOST_CHECK_EQUAL(b,1);
    std::cout << std::endl;

    BOOST_TEST_MESSAGE("Typename tester for concept InnerContainerTypeOfDecay to detect inner container types: ");
    b=InnerContainerTypeOfDecay<int,std::vector<int>>;
    BOOST_CHECK_EQUAL(b,1);
    b=InnerContainerTypeOfDecay<int,int>;
    BOOST_CHECK_EQUAL(b,0);
    b=InnerContainerTypeOfDecay<std::vector<int>,std::vector<int>>;
    BOOST_CHECK_EQUAL(b,0);
    b=InnerContainerTypeOfDecay<std::variant<std::vector<int>,is_deque<int>>,std::vector<std::variant<std::vector<int>,is_deque<int>>>>;
    BOOST_CHECK_EQUAL(b,1);
    std::cout << std::endl;

    BOOST_TEST_MESSAGE("Typename tester for concept InnerContainerTypeOfDecay references: ");
    b=InnerContainerTypeOfDecay<int&,std::vector<int>>;
    BOOST_CHECK_EQUAL(b,1);
    b=InnerContainerTypeOfDecay<int&,int>;
    BOOST_CHECK_EQUAL(b,0);
    b=InnerContainerTypeOfDecay<std::vector<int>&,std::vector<int>>;
    BOOST_CHECK_EQUAL(b,0);
    b=InnerContainerTypeOfDecay<std::variant<std::vector<int>,is_deque<int>>&,std::vector<std::variant<std::vector<int>,is_deque<int>>>>;
    BOOST_CHECK_EQUAL(b,1);
    std::cout << std::endl;

    b=InnerContainerTypeOfDecay<int&&,std::vector<int>>;
    BOOST_CHECK_EQUAL(b,1);
    b=InnerContainerTypeOfDecay<int&&,int>;
    BOOST_CHECK_EQUAL(b,0);
    b=InnerContainerTypeOfDecay<std::vector<int>&&,std::vector<int>>;
    BOOST_CHECK_EQUAL(b,0);
    b=InnerContainerTypeOfDecay<std::variant<std::vector<int>,is_deque<int>>&&,std::vector<std::variant<std::vector<int>,is_deque<int>>>>;
    BOOST_CHECK_EQUAL(b,1);
    std::cout << std::endl;

    BOOST_TEST_MESSAGE("Typename tester for concept InnerContainerTypeOfDecay with swapped references: ");
    b=InnerContainerTypeOfDecay<int,std::vector<int&>>;
    BOOST_CHECK_EQUAL(b,1);
    b=InnerContainerTypeOfDecay<int,int&>;
    BOOST_CHECK_EQUAL(b,0);
    b=InnerContainerTypeOfDecay<std::vector<int>,std::vector<int&>>;
    BOOST_CHECK_EQUAL(b,0);
    b=InnerContainerTypeOfDecay<std::variant<std::vector<int>,is_deque<int>>,std::vector<std::variant<std::vector<int>,is_deque<int>>&>>;
    BOOST_CHECK_EQUAL(b,1);
    std::cout << std::endl;

    b=InnerContainerTypeOfDecay<int,std::vector<int&&>>;
    BOOST_CHECK_EQUAL(b,1);
    b=InnerContainerTypeOfDecay<int,int&&>;
    BOOST_CHECK_EQUAL(b,0);
    b=InnerContainerTypeOfDecay<std::vector<int>,std::vector<int&&>>;
    BOOST_CHECK_EQUAL(b,0);
    b=InnerContainerTypeOfDecay<std::variant<std::vector<int>,is_deque<int>>,std::vector<std::variant<std::vector<int>,is_deque<int>>&&>>;
    BOOST_CHECK_EQUAL(b,1);
    std::cout << std::endl;
}

BOOST_AUTO_TEST_CASE( similar_container_test )
{
    BOOST_TEST_MESSAGE("Typename tester for concept SimilarContainer to detect same container wrappers: ");
    bool b=SimilarContainer<std::vector<int>,std::vector<int>>;
    BOOST_CHECK_EQUAL(b,1);
    b=SimilarContainer<std::vector<int>,std::vector<double>>;
    BOOST_CHECK_EQUAL(b,1);
    b=SimilarContainer<std::vector<int>,std::deque<int>>;
    BOOST_CHECK_EQUAL(b,0);
    b=SimilarContainer<std::variant<std::vector<int>,std::deque<double>>,std::deque<int>>;
    BOOST_CHECK_EQUAL(b,0);
    b=SimilarContainer<std::variant<std::vector<int>,std::deque<double>>,std::variant<std::list<double>,std::deque<int>>>;
    BOOST_CHECK_EQUAL(b,0);
    std::cout << std::endl;

    BOOST_TEST_MESSAGE("Typename tester for concept SimilarDeepContainer to detect same container wrappers: ");
    b=SimilarDeepContainer<std::vector<int>,std::vector<int>>;
    BOOST_CHECK_EQUAL(b,1);
    b=SimilarDeepContainer<std::vector<int>,std::vector<double>>;
    BOOST_CHECK_EQUAL(b,1);
    b=SimilarDeepContainer<std::vector<int>,std::deque<int>>;
    BOOST_CHECK_EQUAL(b,0);
    b=SimilarDeepContainer<std::variant<std::vector<int>,std::deque<double>>,std::deque<int>>;
    BOOST_CHECK_EQUAL(b,0);
    b=SimilarDeepContainer<std::variant<std::vector<int>,std::deque<double>>,std::variant<std::list<double>,std::deque<int>>>;
    BOOST_CHECK_EQUAL(b,1);
    std::cout << std::endl;
}