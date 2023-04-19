#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "uhLibChunking TempFile Tests"
#endif

#include <boost/test/unit_test.hpp>
#include <util/exception.h>
#include <chunking/rabin_polynomial.h>



namespace
{

const static std::string LOREM_IPSUM =
    "Lorem ipsum dolor sit amet, consectetur adipiscing "
    "elit, sed do eiusmod tempor incididunt ut labore et "
    "dolore magna aliqua. Ut enim ad minim veniam, quis "
    "nostrud exercitation ullamco laboris nisi ut "
    "aliquip ex ea commodo consequat. Duis aute irure "
    "dolor in reprehenderit in voluptate velit esse "
    "cillum dolore eu fugiat nulla pariatur. Excepteur "
    "sint occaecat cupidatat non proident, sunt in culpa "
    "qui officia deserunt mollit anim id est laborum.";

// ---------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( dummy )
{
    BOOST_CHECK(true);
}

}//namespace
