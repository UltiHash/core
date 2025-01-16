#define BOOST_TEST_MODULE "entrypoint variables"

#include "entrypoint.h"
#include <boost/test/unit_test.hpp>
#include <common/types/common_types.h>
#include <entrypoint/commands/command.h>
#include <entrypoint/http/request.h>

using namespace uh::cluster;
using namespace uh::cluster::test;
using namespace uh::cluster::ep;
using namespace uh::cluster::ep::http;
using namespace uh::cluster::ep::policy;

namespace {

BOOST_AUTO_TEST_CASE(variable_replace) {
    {
        auto result = var_replace("foo", vars({}));
        BOOST_TEST(result == "foo");
    }

    {
        auto result = var_replace("${foo}", vars({{"foo", "bar"}}));
        BOOST_TEST(result == "bar");
    }

    {
        auto result = var_replace("fo${bar}o", vars({}));
        BOOST_TEST(result == "foo");
    }

    {
        auto result = var_replace("${}", vars({}));
        BOOST_TEST(result == "");
    }

    {
        auto result = var_replace("${foo:bar}", vars({{"foo:bar", "baz"}}));
        BOOST_TEST(result == "baz");
    }

    {
        auto result = var_replace("${foo}${", vars({{"foo", "baz"}}));
        BOOST_TEST(result == "baz${");
    }

    {
        auto result =
            var_replace("${foo}${bar}", vars({{"foo", "baz"}, {"bar", "bar"}}));
        BOOST_TEST(result == "bazbar");
    }

    {
        auto result = var_replace("\\${foo}", vars({{"foo", "baz"}}));
        BOOST_TEST(result == "${foo}");
    }

    {
        auto result = var_replace("${${foo}}", vars({{"foo", "baz"}}));
        BOOST_TEST(result == "}");
    }
}

BOOST_AUTO_TEST_CASE(variable_replace__fails_for_wrong_format) {
    {
        auto result = var_replace("{foo}", vars({{"foo", "baz"}}));
        BOOST_TEST(result != "baz");
    }
}

BOOST_AUTO_TEST_CASE(variable_replace__fails_for_wrong_variable_name) {
    {
        auto result = var_replace("${foo}", vars({{"foo2", "baz"}}));
        BOOST_TEST(result != "baz");
    }
}

BOOST_AUTO_TEST_CASE(variable_replace__remap_specialchars_well) {
    {
        auto result = var_replace("${*}", vars({{"foo2", "baz"}}));
        BOOST_TEST(result == "*");
    }
    {
        auto result = var_replace("${?}", vars({{"foo2", "baz"}}));
        BOOST_TEST(result == "?");
    }
    {
        auto result = var_replace("${$}", vars({{"foo2", "baz"}}));
        BOOST_TEST(result == "$");
    }
}

BOOST_AUTO_TEST_CASE(wildcard_match) {
    BOOST_CHECK(equals_wildcard("", ""));
    BOOST_CHECK(equals_wildcard("foo", "foo"));

    BOOST_CHECK(equals_wildcard("foo*", "foo"));
    BOOST_CHECK(equals_wildcard("foo*", "foobar"));
    BOOST_CHECK(equals_wildcard("fo*", "fo"));

    BOOST_CHECK(equals_wildcard("ba?", "baz"));
    BOOST_CHECK(equals_wildcard("ba?", "bar"));
    BOOST_CHECK(equals_wildcard("ba?q", "barq"));

    BOOST_CHECK(equals_wildcard("foo*bar", "foobar"));
    BOOST_CHECK(equals_wildcard("foo*bar", "fooquuxbar"));
    BOOST_CHECK(equals_wildcard("foo*bar", "fooqbarquuxbar"));

    BOOST_CHECK(equals_wildcard("fo*ar*ux", "foobarquux"));
    BOOST_CHECK(equals_wildcard("foo*ba?", "fooquuxbar"));
    BOOST_CHECK(equals_wildcard("foo*ba?", "fooquuxbaz"));
    BOOST_CHECK(equals_wildcard("foo*ba?", "foobaz"));

    BOOST_CHECK(equals_wildcard("???", "foo"));
    BOOST_CHECK(equals_wildcard("*", "foo"));
    BOOST_CHECK(equals_wildcard("*?", "f"));

    BOOST_CHECK(equals_wildcard("foo*${*}", "foo_asdf_*", vars({})));
    BOOST_CHECK(equals_wildcard("foo*${*}end", "foo_asdf_*end", vars({})));
    BOOST_CHECK(
        equals_wildcard("foo*${$}${foo}", "foo_$bar", vars({{"foo", "bar"}})));
}

BOOST_AUTO_TEST_CASE(wildcard_match__fails_on_false_match) {
    BOOST_CHECK(!equals_wildcard("", "bar"));
    BOOST_CHECK(!equals_wildcard("foo", "bar"));
    BOOST_CHECK(!equals_wildcard("*?", ""));
    BOOST_CHECK(!equals_wildcard("root", "arn:foo:root"));

    BOOST_CHECK(!equals_wildcard("foo*bar", "fooqbarxx", vars({})));
}

BOOST_AUTO_TEST_CASE(default_variables) {
    auto user_arn = "arn:aws:iam::2:random_user";
    auto request =
        make_request("GET /bucket/object?delimiter=!!!&prefix=foo HTTP/1.1\r\n"
                     "User-Agent: USER_AGENT\r\n"
                     "X-amz-content-sha256: UNSIGNED-PAYLOAD\r\n"
                     "x-amz-copy-source: COPY_SOURCE\r\n"
                     "Referer: HTTP_REFERER\r\n\r\n",
                     user_arn);

    auto command = mock_command("s3:GetObject");
    variables vars(request, command);

    BOOST_CHECK_EQUAL(vars.get("uh:ActionId").value_or(""), "s3:GetObject");
    BOOST_CHECK_EQUAL(vars.get("uh:actionid").value_or(""), "s3:GetObject");

    BOOST_CHECK_EQUAL(vars.get("uh:ResourceArn").value_or(""),
                      "arn:aws:s3:::bucket/object");
    BOOST_CHECK_EQUAL(vars.get("uh:resourcearn").value_or(""),
                      "arn:aws:s3:::bucket/object");

    BOOST_CHECK_EQUAL(vars.get("aws:PrincipalArn").value_or(""), user_arn);
    BOOST_CHECK_EQUAL(vars.get("aws:principalarn").value_or(""), user_arn);

    BOOST_CHECK_EQUAL(vars.get("aws:userid").value_or("non-empty"), "");
    BOOST_CHECK_EQUAL(vars.get("aws:UsErId").value_or("non-empty"), "");

    BOOST_CHECK_EQUAL(vars.get("aws:SourceIp").value_or(""), "0.0.0.0");
    BOOST_CHECK_EQUAL(vars.get("aws:SoUrCeIp").value_or(""), "0.0.0.0");

    BOOST_CHECK_EQUAL(vars.get("aws:referer").value_or(""), "HTTP_REFERER");
    BOOST_CHECK_EQUAL(vars.get("aws:ReFeReR").value_or(""), "HTTP_REFERER");

    BOOST_CHECK_EQUAL(vars.get("aws:UserAgent").value_or(""), "USER_AGENT");
    BOOST_CHECK_EQUAL(vars.get("aws:usEragEnT").value_or(""), "USER_AGENT");

    BOOST_CHECK_EQUAL(vars.get("s3:x-amz-content-sha256").value_or(""),
                      "UNSIGNED-PAYLOAD");
    BOOST_CHECK_EQUAL(vars.get("s3:X-AmZ-CoNtEnT-ShA256").value_or(""),
                      "UNSIGNED-PAYLOAD");

    BOOST_CHECK_EQUAL(vars.get("s3:x-amz-copy-source").value_or(""),
                      "COPY_SOURCE");
    BOOST_CHECK_EQUAL(vars.get("s3:X-AmZ-CoPy-sOuRcE").value_or(""),
                      "COPY_SOURCE");

    BOOST_CHECK_EQUAL(vars.get("s3:delimiter").value_or(""), "!!!");
    BOOST_CHECK_EQUAL(vars.get("s3:DeLiMiTeR").value_or(""), "!!!");

    BOOST_CHECK_EQUAL(vars.get("s3:prefix").value_or(""), "foo");
    BOOST_CHECK_EQUAL(vars.get("s3:PrEfIx").value_or(""), "foo");
}

} // namespace
