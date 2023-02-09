#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "ultitree"
#endif

#include <boost/test/unit_test.hpp>
#include <trees/ultitree.h>
#include <trees/ut_tree.h>


using namespace uh::trees;

std::string to_string(std::span<const char> buffer)
{
    return std::string(buffer.begin(), buffer.end());
}

template <typename ultitree>
void generic_test_paths(ultitree& ut)
{
    std::string text1 = "QWERFDSA";
    auto path1 = ut.insert({text1.begin(), text1.end()});
    BOOST_TEST(path1.size() == 1u);
    BOOST_TEST(to_string(path1.front()->data) == std::string("QWERFDSA"));

    std::string text2 = "QWERSDFA";
    auto path2 = ut.insert({text2.begin(), text2.end()});

    BOOST_TEST(path2.size() == 2u);
    BOOST_TEST(to_string(path2.front()->data) == std::string("QWER"));
    BOOST_TEST(to_string((*std::next(path2.begin()))->data) == std::string("SDFA"));

    std::string text3 = "SDFA";
    auto path3 = ut.insert({ text3.begin(), text3.end() });

    BOOST_TEST(path3.size() == 1u);
    BOOST_TEST(to_string(path3.front()->data) == std::string("SDFA"));
}

BOOST_AUTO_TEST_CASE(list_paths)
{
    ultitree_list ut;
    generic_test_paths(ut);
}

BOOST_AUTO_TEST_CASE(tree_paths)
{
    ultitree_tree ut;
    generic_test_paths(ut);
}
