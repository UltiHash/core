//
// Created by benjamin-elias on 26.11.22.
//

#define BOOST_TEST_DYN_LINK
#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "uhLibConceptTypes container filtering Tests"
#endif

#include <boost/test/unit_test.hpp>
#include <conceptTypes/conceptTypes.h>

// ------------- Tests Follow --------------

BOOST_AUTO_TEST_CASE( vector_test )
{
    BOOST_TEST_MESSAGE("Typename tester for concept Vector: ");
    BOOST_CHECK_EQUAL(Vector<std::vector<int>>,1);
    BOOST_CHECK_EQUAL(Vector<boost::container::vector<int>>,1);
    BOOST_CHECK_EQUAL(Vector<boost::container::stable_vector<int>>,1);
    BOOST_CHECK_EQUAL(Vector<std::deque<int>>,0);
    BOOST_CHECK_EQUAL(Vector<int>,0);
    std::cout << std::endl;
    BOOST_TEST_MESSAGE("Typename tester for concept Vector with references for std::vector: ");
    BOOST_CHECK_EQUAL(Vector<std::vector<int>>,1);
    BOOST_CHECK_EQUAL(Vector<std::vector<int>&>,1);
    BOOST_CHECK_EQUAL(Vector<std::vector<int>&&>,1);
    BOOST_CHECK_EQUAL(Vector<const std::vector<int>>,1);
    BOOST_CHECK_EQUAL(Vector<const std::vector<int>&>,1);
    BOOST_CHECK_EQUAL(Vector<const std::vector<int>&&>,1);
    std::cout << std::endl;
    BOOST_TEST_MESSAGE("Typename tester for concept Vector with references for boost::container::vector: ");
    BOOST_CHECK_EQUAL(Vector<boost::container::vector<int>>,1);
    BOOST_CHECK_EQUAL(Vector<boost::container::vector<int>&>,1);
    BOOST_CHECK_EQUAL(Vector<boost::container::vector<int>&&>,1);
    BOOST_CHECK_EQUAL(Vector<const boost::container::vector<int>>,1);
    BOOST_CHECK_EQUAL(Vector<const boost::container::vector<int>&>,1);
    BOOST_CHECK_EQUAL(Vector<const boost::container::vector<int>&&>,1);
    std::cout << std::endl;
    BOOST_TEST_MESSAGE("Typename tester for concept Vector with references for boost::container::stable_vector: ");
    BOOST_CHECK_EQUAL(Vector<boost::container::stable_vector<int>>,1);
    BOOST_CHECK_EQUAL(Vector<boost::container::stable_vector<int>&>,1);
    BOOST_CHECK_EQUAL(Vector<boost::container::stable_vector<int>&&>,1);
    BOOST_CHECK_EQUAL(Vector<const boost::container::stable_vector<int>>,1);
    BOOST_CHECK_EQUAL(Vector<const boost::container::stable_vector<int>&>,1);
    BOOST_CHECK_EQUAL(Vector<const boost::container::stable_vector<int>&&>,1);
    std::cout << std::endl;
    BOOST_TEST_MESSAGE("Typename tester for concept VectorStandard with references for boost::container::stable_vector: ");
    BOOST_CHECK_EQUAL(VectorStandard<std::vector<int>>,1);
    BOOST_CHECK_EQUAL(VectorStandard<boost::container::vector<int>>,1);
    BOOST_CHECK_EQUAL(VectorStandard<boost::container::stable_vector<int>>,0);
    std::cout << std::endl;
}

BOOST_AUTO_TEST_CASE( deque_test )
{
    BOOST_TEST_MESSAGE("Typename tester for concept Deque: ");
    BOOST_CHECK_EQUAL(Deque<std::deque<int>>,1);
    BOOST_CHECK_EQUAL(Deque<boost::container::deque<int>>,1);
    BOOST_CHECK_EQUAL(Deque<boost::container::devector<int>>,1);
    BOOST_CHECK_EQUAL(Deque<std::vector<int>>,0);
    BOOST_CHECK_EQUAL(Deque<int>,0);
    std::cout << std::endl;
    BOOST_TEST_MESSAGE("Typename tester for concept Deque with references for std::deque: ");
    BOOST_CHECK_EQUAL(Deque<std::deque<int>>,1);
    BOOST_CHECK_EQUAL(Deque<std::deque<int>&>,1);
    BOOST_CHECK_EQUAL(Deque<std::deque<int>&&>,1);
    BOOST_CHECK_EQUAL(Deque<const std::deque<int>>,1);
    BOOST_CHECK_EQUAL(Deque<const std::deque<int>&>,1);
    BOOST_CHECK_EQUAL(Deque<const std::deque<int>&&>,1);
    std::cout << std::endl;
    BOOST_TEST_MESSAGE("Typename tester for concept Deque with references for boost::container::deque: ");
    BOOST_CHECK_EQUAL(Deque<boost::container::deque<int>>,1);
    BOOST_CHECK_EQUAL(Deque<boost::container::deque<int>&>,1);
    BOOST_CHECK_EQUAL(Deque<boost::container::deque<int>&&>,1);
    BOOST_CHECK_EQUAL(Deque<const boost::container::deque<int>>,1);
    BOOST_CHECK_EQUAL(Deque<const boost::container::deque<int>&>,1);
    BOOST_CHECK_EQUAL(Deque<const boost::container::deque<int>&&>,1);
    std::cout << std::endl;
    BOOST_TEST_MESSAGE("Typename tester for concept Deque with references for boost::container::devector: ");
    BOOST_CHECK_EQUAL(Deque<boost::container::devector<int>>,1);
    BOOST_CHECK_EQUAL(Deque<boost::container::devector<int>&>,1);
    BOOST_CHECK_EQUAL(Deque<boost::container::devector<int>&&>,1);
    BOOST_CHECK_EQUAL(Deque<const boost::container::devector<int>>,1);
    BOOST_CHECK_EQUAL(Deque<const boost::container::devector<int>&>,1);
    BOOST_CHECK_EQUAL(Deque<const boost::container::devector<int>&&>,1);
    std::cout << std::endl;
}

BOOST_AUTO_TEST_CASE( forward_list_test )
{
    BOOST_TEST_MESSAGE("Typename tester for concept ForwardList: ");
    BOOST_CHECK_EQUAL(ForwardList<std::forward_list<int>>,1);
    BOOST_CHECK_EQUAL(ForwardList<boost::container::slist<int>>,1);
    BOOST_CHECK_EQUAL(ForwardList<std::vector<int>>,0);
    BOOST_CHECK_EQUAL(ForwardList<int>,0);
    std::cout << std::endl;
    BOOST_TEST_MESSAGE("Typename tester for concept ForwardList with references for std::forward_list: ");
    BOOST_CHECK_EQUAL(ForwardList<std::forward_list<int>>,1);
    BOOST_CHECK_EQUAL(ForwardList<std::forward_list<int>&>,1);
    BOOST_CHECK_EQUAL(ForwardList<std::forward_list<int>&&>,1);
    BOOST_CHECK_EQUAL(ForwardList<const std::forward_list<int>>,1);
    BOOST_CHECK_EQUAL(ForwardList<const std::forward_list<int>&>,1);
    BOOST_CHECK_EQUAL(ForwardList<const std::forward_list<int>&&>,1);
    std::cout << std::endl;
    BOOST_TEST_MESSAGE("Typename tester for concept ForwardList with references for boost::container::slist: ");
    BOOST_CHECK_EQUAL(ForwardList<boost::container::slist<int>>,1);
    BOOST_CHECK_EQUAL(ForwardList<boost::container::slist<int>&>,1);
    BOOST_CHECK_EQUAL(ForwardList<boost::container::slist<int>&&>,1);
    BOOST_CHECK_EQUAL(ForwardList<const boost::container::slist<int>>,1);
    BOOST_CHECK_EQUAL(ForwardList<const boost::container::slist<int>&>,1);
    BOOST_CHECK_EQUAL(ForwardList<const boost::container::slist<int>&&>,1);
    std::cout << std::endl;
}

BOOST_AUTO_TEST_CASE( list_test )
{
    BOOST_TEST_MESSAGE("Typename tester for concept List: ");
    BOOST_CHECK_EQUAL(List<std::list<int>>,1);
    BOOST_CHECK_EQUAL(List<boost::container::list<int>>,1);
    BOOST_CHECK_EQUAL(List<std::vector<int>>,0);
    BOOST_CHECK_EQUAL(List<int>,0);
    std::cout << std::endl;
    BOOST_TEST_MESSAGE("Typename tester for concept List with references for std::list: ");
    BOOST_CHECK_EQUAL(List<std::list<int>>,1);
    BOOST_CHECK_EQUAL(List<std::list<int>&>,1);
    BOOST_CHECK_EQUAL(List<std::list<int>&&>,1);
    BOOST_CHECK_EQUAL(List<const std::list<int>>,1);
    BOOST_CHECK_EQUAL(List<const std::list<int>&>,1);
    BOOST_CHECK_EQUAL(List<const std::list<int>&&>,1);
    std::cout << std::endl;
    BOOST_TEST_MESSAGE("Typename tester for concept List with references for boost::container::list: ");
    BOOST_CHECK_EQUAL(List<boost::container::list<int>>,1);
    BOOST_CHECK_EQUAL(List<boost::container::list<int>&>,1);
    BOOST_CHECK_EQUAL(List<boost::container::list<int>&&>,1);
    BOOST_CHECK_EQUAL(List<const boost::container::list<int>>,1);
    BOOST_CHECK_EQUAL(List<const boost::container::list<int>&>,1);
    BOOST_CHECK_EQUAL(List<const boost::container::list<int>&&>,1);
    std::cout << std::endl;
}

BOOST_AUTO_TEST_CASE( container_test )
{
    BOOST_TEST_MESSAGE("Typename tester for concept SequentialContainer: ");
    BOOST_CHECK_EQUAL(SequentialContainer<std::vector<int>>, 1);
    BOOST_CHECK_EQUAL(SequentialContainer<boost::container::vector<int>>, 1);
    BOOST_CHECK_EQUAL(SequentialContainer<boost::container::stable_vector<int>>, 1);
    BOOST_CHECK_EQUAL(SequentialContainer<std::deque<int>>, 1);
    BOOST_CHECK_EQUAL(SequentialContainer<boost::container::deque<int>>, 1);
    BOOST_CHECK_EQUAL(SequentialContainer<boost::container::devector<int>>, 1);
    BOOST_CHECK_EQUAL(SequentialContainer<std::forward_list<int>>, 1);
    BOOST_CHECK_EQUAL(SequentialContainer<boost::container::slist<int>>, 1);
    BOOST_CHECK_EQUAL(SequentialContainer<std::list<int>>, 1);
    BOOST_CHECK_EQUAL(SequentialContainer<boost::container::list<int>>, 1);
    BOOST_TEST_MESSAGE("But those types don't work: ");
    BOOST_CHECK_EQUAL(SequentialContainer<int>, 0);
    bool b=SequentialContainer<std::variant<int,double>>;
    BOOST_CHECK_EQUAL(b,0);
    BOOST_TEST_MESSAGE("Reference example: ");
    b=SequentialContainer<std::vector<int>&>;
    BOOST_CHECK_EQUAL(b,1);
    b=SequentialContainer<std::vector<int>&&>;
    BOOST_CHECK_EQUAL(b,1);
    std::cout << std::endl;
}

BOOST_AUTO_TEST_CASE( iterator_type_test ){
    BOOST_TEST_MESSAGE("Typename tester for concept IteratorType: ");
    BOOST_CHECK_EQUAL(IteratorType<std::vector<int>::iterator>,1);
    BOOST_CHECK_EQUAL(IteratorType<boost::container::vector<int>::iterator>,1);
    BOOST_CHECK_EQUAL(IteratorType<boost::container::stable_vector<int>::iterator>,1);
    BOOST_CHECK_EQUAL(IteratorType<std::deque<int>::iterator>,1);
    BOOST_CHECK_EQUAL(IteratorType<boost::container::deque<int>::iterator>,1);
    BOOST_CHECK_EQUAL(IteratorType<boost::container::devector<int>::iterator>,1);
    BOOST_CHECK_EQUAL(IteratorType<std::forward_list<int>::iterator>,1);
    BOOST_CHECK_EQUAL(IteratorType<boost::container::slist<int>::iterator>,1);
    BOOST_CHECK_EQUAL(IteratorType<std::list<int>::iterator>,1);
    BOOST_CHECK_EQUAL(IteratorType<boost::container::list<int>::iterator>,1);
    BOOST_TEST_MESSAGE("But those types don't work: ");
    BOOST_CHECK_EQUAL(IteratorType<int>,0);
    bool b=IteratorType<std::variant<int,double>>;
    BOOST_CHECK_EQUAL(b,0);
    BOOST_TEST_MESSAGE("Reference example: ");
    b=IteratorType<std::vector<int>::iterator&>;
    BOOST_CHECK_EQUAL(b,1);
    b=IteratorType<std::vector<int>::iterator&&>;
    BOOST_CHECK_EQUAL(b,1);
    BOOST_TEST_MESSAGE("Other examples: ");
    b=IteratorType<int>;
    BOOST_CHECK_EQUAL(b,0);
    b=IteratorType<int*>;
    BOOST_CHECK_EQUAL(b,1);
    b=IteratorType<std::vector<int>>;
    BOOST_CHECK_EQUAL(b,0);
    std::cout << std::endl;
    BOOST_TEST_MESSAGE("Typename tester for concept IteratorType for map iterators: ");
    b=IteratorType<std::map<int,std::string>::iterator>;
    BOOST_CHECK_EQUAL(b,1);
    b=IteratorType<boost::unordered_map<int,std::string>::iterator>;
    BOOST_CHECK_EQUAL(b,1);
    b=IteratorType<boost::container::map<int,std::string>::iterator>;
    BOOST_CHECK_EQUAL(b,1);
    std::cout << std::endl;
}

