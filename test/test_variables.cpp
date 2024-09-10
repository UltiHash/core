#define BOOST_TEST_MODULE "entrypoint variables"

#include <boost/test/unit_test.hpp>
#include <entrypoint/variables.h>

using namespace uh::cluster::ep;

namespace {

BOOST_AUTO_TEST_CASE(variable_replace) {
    {
        auto result = var_replace("foo", {});
        BOOST_CHECK(result == "foo");
    }

    {
        auto result = var_replace("${foo}", {{"foo", "bar"}});
        BOOST_CHECK(result == "bar");
    }

    {
        auto result = var_replace("fo${bar}o", {});
        BOOST_CHECK(result == "foo");
    }

    {
        auto result = var_replace("${}", {});
        BOOST_CHECK(result == "");
    }

    {
        auto result = var_replace("${foo:bar}", {{"foo:bar", "baz"}});
        BOOST_CHECK(result == "baz");
    }

    {
        auto result = var_replace("${foo}${", {{"foo", "baz"}});
        BOOST_CHECK(result == "baz${");
    }

    {
        auto result =
            var_replace("${foo}${bar}", {{"foo", "baz"}, {"bar", "bar"}});
        BOOST_CHECK(result == "bazbar");
    }

    {
        auto result = var_replace("\\${foo}", {{"foo", "baz"}});
        BOOST_CHECK(result == "${foo}");
    }

    {
        auto result = var_replace("${${foo}}", {{"foo", "baz"}});
        BOOST_CHECK(result == "}");
    }
}

} // namespace
