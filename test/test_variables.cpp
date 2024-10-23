#define BOOST_TEST_MODULE "entrypoint variables"

#include <boost/test/unit_test.hpp>
#include <common/types/common_types.h>
#include <entrypoint/commands/command.h>
#include <entrypoint/http/request.h>
#include <entrypoint/policy/variables.h>
#include <test/http_request.h>

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

BOOST_AUTO_TEST_CASE(default_variables) {
    auto user_arn = "arn:aws:iam::2:random_user";
    auto request = test::make_request(
        "GET /bucket/object?delimiter=!!!&prefix=foo HTTP/1.1\r\n"
        "User-Agent: USER_AGENT\r\n"
        "X-amz-content-sha256: UNSIGNED-PAYLOAD\r\n"
        "x-amz-copy-source: COPY_SOURCE\r\n"
        "Referer: HTTP_REFERER\r\n\r\n",
        user_arn);

    auto command = test::mock_command("s3:GetObject");
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
