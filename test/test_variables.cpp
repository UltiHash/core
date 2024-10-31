#define BOOST_TEST_MODULE "entrypoint variables"

#include <boost/test/unit_test.hpp>
#include <common/types/common_types.h>
#include <entrypoint/commands/command.h>
#include <entrypoint/http/request.h>
#include <test/entrypoint.h>

using namespace uh::cluster;
using namespace uh::cluster::test;
using namespace uh::cluster::ep;
using namespace uh::cluster::ep::http;
using namespace uh::cluster::ep::policy;

namespace {

BOOST_AUTO_TEST_CASE(variable_replace) {
    {
        auto result = var_replace(remap_wildcards("foo"), vars({}));
        BOOST_TEST(result == "foo");
    }

    {
        auto result =
            var_replace(remap_wildcards("${foo}"), vars({{"foo", "bar"}}));
        BOOST_TEST(result == "bar");
    }

    {
        auto result = var_replace(remap_wildcards("fo${bar}o"), vars({}));
        BOOST_TEST(result == "foo");
    }

    {
        auto result = var_replace(remap_wildcards("${}"), vars({}));
        BOOST_TEST(result == "");
    }

    {
        auto result = var_replace(remap_wildcards("${foo:bar}"),
                                  vars({{"foo:bar", "baz"}}));
        BOOST_TEST(result == "baz");
    }

    {
        auto result =
            var_replace(remap_wildcards("${foo}${"), vars({{"foo", "baz"}}));
        BOOST_TEST(result == "baz${");
    }

    {
        auto result = var_replace(remap_wildcards("${foo}${bar}"),
                                  vars({{"foo", "baz"}, {"bar", "bar"}}));
        BOOST_TEST(result == "bazbar");
    }

    {
        auto result =
            var_replace(remap_wildcards("\\${foo}"), vars({{"foo", "baz"}}));
        BOOST_TEST(result == "${foo}");
    }

    {
        auto result =
            var_replace(remap_wildcards("${${foo}}"), vars({{"foo", "baz"}}));
        BOOST_TEST(result == "}");
    }
}

BOOST_AUTO_TEST_CASE(variable_replace__fails_for_wrong_format) {
    {
        auto result =
            var_replace(remap_wildcards("{foo}"), vars({{"foo", "baz"}}));
        BOOST_TEST(result != "baz");
    }
}

BOOST_AUTO_TEST_CASE(variable_replace__fails_for_wrong_variable_name) {
    {
        auto result =
            var_replace(remap_wildcards("${foo}"), vars({{"foo2", "baz"}}));
        BOOST_TEST(result != "baz");
    }
}

BOOST_AUTO_TEST_CASE(variable_replace__remap_specialchars_well) {
    {
        auto result =
            var_replace(remap_wildcards("${*}"), vars({{"foo2", "baz"}}));
        BOOST_TEST(result == "*");
    }
    {
        auto result =
            var_replace(remap_wildcards("${?}"), vars({{"foo2", "baz"}}));
        BOOST_TEST(result == "?");
    }
    {
        auto result =
            var_replace(remap_wildcards("${$}"), vars({{"foo2", "baz"}}));
        BOOST_TEST(result == "$");
    }
}

BOOST_AUTO_TEST_CASE(wildcard_match) {
    BOOST_CHECK(equals_wildcard(remap_wildcards(""), ""));
    BOOST_CHECK(equals_wildcard(remap_wildcards("foo"), "foo"));

    BOOST_CHECK(equals_wildcard(remap_wildcards("foo*"), "foo"));
    BOOST_CHECK(equals_wildcard(remap_wildcards("foo*"), "foobar"));
    BOOST_CHECK(equals_wildcard(remap_wildcards("fo*"), "fo"));

    BOOST_CHECK(equals_wildcard(remap_wildcards("ba?"), "baz"));
    BOOST_CHECK(equals_wildcard(remap_wildcards("ba?"), "bar"));
    BOOST_CHECK(equals_wildcard(remap_wildcards("ba?q"), "barq"));

    BOOST_CHECK(equals_wildcard(remap_wildcards("foo*bar"), "foobar"));
    BOOST_CHECK(equals_wildcard(remap_wildcards("foo*bar"), "fooquuxbar"));
    BOOST_CHECK(equals_wildcard(remap_wildcards("foo*bar"), "fooqbarquuxbar"));

    BOOST_CHECK(equals_wildcard(remap_wildcards("fo*ar*ux"), "foobarquux"));
    BOOST_CHECK(equals_wildcard(remap_wildcards("foo*ba?"), "fooquuxbar"));
    BOOST_CHECK(equals_wildcard(remap_wildcards("foo*ba?"), "fooquuxbaz"));
    BOOST_CHECK(equals_wildcard(remap_wildcards("foo*ba?"), "foobaz"));

    BOOST_CHECK(equals_wildcard(remap_wildcards("???"), "foo"));
    BOOST_CHECK(equals_wildcard(remap_wildcards("*"), "foo"));
    BOOST_CHECK(equals_wildcard(remap_wildcards("*?"), "f"));

    BOOST_CHECK(equals_wildcard(
        var_replace(remap_wildcards("foo*${*}"), vars({})), "foo_asdf_*"));
    BOOST_CHECK(
        equals_wildcard(var_replace(remap_wildcards("foo*${*}end"), vars({})),
                        "foo_asdf_*end"));
    BOOST_CHECK(equals_wildcard(
        var_replace(remap_wildcards("foo*${$}${foo}"), vars({{"foo", "bar"}})),
        "foo_$bar"));
}

BOOST_AUTO_TEST_CASE(wildcard_match__fails_on_false_match) {
    BOOST_CHECK(!equals_wildcard(remap_wildcards(""), "bar"));
    BOOST_CHECK(!equals_wildcard(remap_wildcards("foo"), "bar"));
    BOOST_CHECK(!equals_wildcard(remap_wildcards("*?"), ""));
    BOOST_CHECK(!equals_wildcard(remap_wildcards("root"), "arn:foo:root"));

    BOOST_CHECK(!equals_wildcard(
        var_replace(remap_wildcards("foo*bar"), vars({})), "fooqbarxx"));
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
