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

BOOST_AUTO_TEST_CASE(wildcard_match) {
    BOOST_CHECK(equals_wildcard("", ""));
    BOOST_CHECK(!equals_wildcard("", "bar"));
    BOOST_CHECK(equals_wildcard("foo", "foo"));
    BOOST_CHECK(!equals_wildcard("foo", "bar"));

    BOOST_CHECK(equals_wildcard("foo*", "foo"));
    BOOST_CHECK(equals_wildcard("foo*", "foobar"));
    BOOST_CHECK(equals_wildcard("fo*", "fo"));

    BOOST_CHECK(equals_wildcard("ba?", "baz"));
    BOOST_CHECK(equals_wildcard("ba?", "bar"));
    BOOST_CHECK(equals_wildcard("ba?q", "barq"));

    BOOST_CHECK(equals_wildcard("foo*bar", "foobar"));
    BOOST_CHECK(equals_wildcard("foo*bar", "fooquuxbar"));

    BOOST_CHECK(equals_wildcard("fo*ar*ux", "foobarquux"));
    BOOST_CHECK(equals_wildcard("foo*ba?", "fooquuxbar"));
    BOOST_CHECK(equals_wildcard("foo*ba?", "fooquuxbaz"));
    BOOST_CHECK(equals_wildcard("foo*ba?", "foobaz"));

    BOOST_CHECK(equals_wildcard("???", "foo"));
    BOOST_CHECK(equals_wildcard("*", "foo"));
    BOOST_CHECK(equals_wildcard("*?", "f"));
    BOOST_CHECK(!equals_wildcard("*?", ""));
}

} // namespace
