#define BOOST_TEST_MODULE "trace tests"

#include <boost/test/unit_test.hpp>

#include "test_config.h"

#include <common/types/common_types.h>

using namespace uh::cluster;
using namespace boost::asio;

BOOST_AUTO_TEST_SUITE(a_http_client)

BOOST_AUTO_TEST_CASE(can_get_response) {
    static constexpr const char* full_function_name =
        "coro<size_t> uh::cluster::(anonymous namespace)::match_size(context "
        "&, dd::cache &, std::string_view, auto) [frag:auto = "
        "std::optional<std::pair<uh::cluster::fragment, "
        "std::basic_string<char>>>]";

    auto function_name = trace_span::extract_function_name(full_function_name);

    std::cout << function_name << std::endl;
    BOOST_TEST(function_name ==
               "uh::cluster::(anonymous namespace)::match_size");
}

BOOST_AUTO_TEST_SUITE_END()
