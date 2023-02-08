#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "bplus"
#endif

#include <boost/test/unit_test.hpp>
#include <trees/bplus.h>


using namespace uh::trees;

std::string to_string(std::span<const char> buffer)
{
    return std::string(buffer.begin(), buffer.end());
}

BOOST_AUTO_TEST_CASE(paths)
{
    bplus bp;

    std::string text1 = "QWERFDSA";
    auto path1 = bp.insert({text1.begin(), text1.end()});
    BOOST_TEST(path1.size() == 1u);
    BOOST_TEST(to_string(path1.front()->data) == std::string("QWERFDSA"));

    std::string text2 = "QWERSDFA";
    auto path2 = bp.insert({text2.begin(), text2.end()});

    BOOST_TEST(path2.size() == 2u);
    BOOST_TEST(to_string(path2.front()->data) == std::string("QWER"));
    BOOST_TEST(to_string((*std::next(path2.begin()))->data) == std::string("SDFA"));

    std::string text3 = "SDFA";
    auto path3 = bp.insert({ text3.begin(), text3.end() });

    BOOST_TEST(path3.size() == 1u);
    BOOST_TEST(to_string(path3.front()->data) == std::string("SDFA"));
}
