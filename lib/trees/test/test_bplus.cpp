#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "bplus"
#endif

#include <boost/test/unit_test.hpp>
#include <trees/bplus.h>


using namespace uh::trees;

std::string to_string(std::span<char> buffer)
{
    return std::string(buffer.begin(), buffer.end());
}

BOOST_AUTO_TEST_CASE(bplus_load_tree)
{
    dictionary dict;

    std::string text3 = "QWERFDSA";
    auto path3 = dict.insert({text3.begin(), text3.end()});
    BOOST_TEST(path3.size() == 1u);
    BOOST_TEST(path3.front()->size() == 8u);
    BOOST_TEST(to_string(path3.front()->data) == std::string("QWERFDSA"));

    std::string text7 = "QWERSDFA";
    auto path7 = dict.insert({text7.begin(), text7.end()});

    BOOST_TEST(path7.size() == 2u);
    BOOST_TEST(path7.front()->size() == 4u);
    BOOST_TEST(to_string(path7.front()->data) == std::string("QWER"));

    BOOST_TEST((*std::next(path7.begin()))->size() == 4u);
    BOOST_TEST(to_string((*std::next(path7.begin()))->data) == std::string("SDFA"));
}
