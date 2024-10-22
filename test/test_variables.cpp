#define BOOST_TEST_MODULE "entrypoint variables"

#include <boost/test/unit_test.hpp>
#include <common/types/common_types.h>
#include <entrypoint/commands/command.h>
#include <entrypoint/http/request.h>
#include <entrypoint/policy/variables.h>

using namespace uh::cluster;
using namespace uh::cluster::ep;
using namespace uh::cluster::ep::http;
using namespace uh::cluster::ep::policy;

namespace {

struct mock_command : public uh::cluster::command {
    coro<response> handle(request&) override { co_return response{}; }
    coro<void> validate(const request& req) override { co_return; }
    std::string action_id() const override { return ""; }
};

variables vars(std::initializer_list<std::pair<std::string, std::string>> v) {
    static http::request req;
    static mock_command cmd;
    variables rv(req, cmd);

    for (auto& p : v) {
        rv.put(std::move(p.first), std::move(p.second));
    }

    return rv;
}

BOOST_AUTO_TEST_CASE(variable_replace) {
    {
        auto result = var_replace("foo", vars({}));
        BOOST_CHECK(result == "foo");
    }

    {
        auto result = var_replace("${foo}", vars({{"foo", "bar"}}));
        BOOST_CHECK(result == "bar");
    }

    {
        auto result = var_replace("fo${bar}o", vars({}));
        BOOST_CHECK(result == "foo");
    }

    {
        auto result = var_replace("${}", vars({}));
        BOOST_CHECK(result == "");
    }

    {
        auto result = var_replace("${foo:bar}", vars({{"foo:bar", "baz"}}));
        BOOST_CHECK(result == "baz");
    }

    {
        auto result = var_replace("${foo}${", vars({{"foo", "baz"}}));
        BOOST_CHECK(result == "baz${");
    }

    {
        auto result =
            var_replace("${foo}${bar}", vars({{"foo", "baz"}, {"bar", "bar"}}));
        BOOST_CHECK(result == "bazbar");
    }

    {
        auto result = var_replace("\\${foo}", vars({{"foo", "baz"}}));
        BOOST_CHECK(result == "${foo}");
    }

    {
        auto result = var_replace("${${foo}}", vars({{"foo", "baz"}}));
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
    BOOST_CHECK(!equals_wildcard("root", "arn:foo:root"));
}

} // namespace
