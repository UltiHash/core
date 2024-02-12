#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "strings tests"
#endif

#include <boost/test/unit_test.hpp>
#include "common/utils/strings.h"

namespace uh::cluster {

BOOST_AUTO_TEST_CASE(string_split) {
    BOOST_CHECK(split("").empty());
    BOOST_CHECK(split("abc").size() == 1);

    {
        auto v = split("abc def gih");
        BOOST_CHECK(v.size() == 3);
        BOOST_CHECK(v[0] == "abc");
        BOOST_CHECK(v[1] == "def");
        BOOST_CHECK(v[2] == "gih");
    }

    {
        auto v = split("abc-def-gih", '-');
        BOOST_CHECK(v.size() == 3);
        BOOST_CHECK(v[0] == "abc");
        BOOST_CHECK(v[1] == "def");
        BOOST_CHECK(v[2] == "gih");
    }
}

}
