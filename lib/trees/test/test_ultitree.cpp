#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "ultitree"
#endif

#include <boost/test/unit_test.hpp>
#include <trees/ultitree.h>
#include <trees/ut_tree.h>


using namespace uh::trees;

template <typename ultitree>
void generic_path_validity(ultitree& ut)
{
    std::string text1 = "QWERFDSA";
    auto hash1 = ut.insert({text1.begin(), text1.end()});

    std::string text2 = "QWERSDFA";
    auto hash2 = ut.insert({text2.begin(), text2.end()});

    std::string text3 = "SDFA";
    auto hash3 = ut.insert({ text3.begin(), text3.end() });

    BOOST_TEST(ut.find(hash1) == text1);
    BOOST_TEST(ut.find(hash2) == text2);
    BOOST_TEST(ut.find(hash3) == text3);
}

template <typename ultitree>
void generic_test_paths(ultitree& ut)
{
    std::string text1 = "QWERFDSA";
    auto hash1 = ut.insert({text1.begin(), text1.end()});
    BOOST_TEST(ut.find(hash1) == text1);

    std::string text2 = "QWERSDFA";
    auto hash2 = ut.insert({text2.begin(), text2.end()});
    BOOST_TEST(ut.find(hash2) == text2);

    std::string text3 = "SDFA";
    auto hash3 = ut.insert({ text3.begin(), text3.end() });
    BOOST_TEST(ut.find(hash3) == text3);
}

BOOST_AUTO_TEST_CASE(list_paths)
{
    ultitree_list ut;
    generic_test_paths(ut);
    generic_path_validity(ut);
}

BOOST_AUTO_TEST_CASE(tree_paths)
{
    ultitree_tree ut;
    generic_test_paths(ut);
    generic_path_validity(ut);
}
