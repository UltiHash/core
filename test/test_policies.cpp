#define BOOST_TEST_MODULE "policy JSON parser tests"

#include "entrypoint/commands/command.h"
#include "entrypoint/policy/parser.h"
#include <boost/test/unit_test.hpp>

// ------------- Tests Suites Follow --------------

using namespace uh::cluster;
using namespace uh::cluster::ep::policy;
using namespace uh::cluster::ep::user;

namespace {

class mock_command : public uh::cluster::command {
public:
    mock_command(const std::string& id)
        : m_id(id) {}
    coro<http_response> handle(http_request&) override {
        co_return http_response{};
    }
    coro<void> validate(const http_request& req) override { co_return; }
    std::string action_id() const override { return m_id; }

private:
    std::string m_id;
};

class mock_body : public uh::cluster::ep::http::body {
public:
    coro<std::size_t> read(std::span<char>) override { co_return 0ull; }
};

auto make_request(const std::string& code,
                  const std::string& principal = user::ANONYMOUS_ARN) {
    boost::beast::http::request_parser<boost::beast::http::empty_body> parser;
    boost::beast::error_code ec;

    parser.put(boost::asio::buffer(code), ec);

    auto req = http_request(parser.get(), std::make_unique<mock_body>(),
                            boost::asio::ip::tcp::endpoint());

    req.authenticated_user(user{.arn = principal});
    return req;
}

} // namespace

BOOST_AUTO_TEST_CASE(check_action) {
    auto policy = parser::parse("{\n"
                                "   \"Version\": \"2012-10-17\",\n"
                                "   \"Statement\": {\n"
                                "       \"Sid\": \"AllowAllForGetObject\",\n"
                                "       \"Effect\": \"Allow\",\n"
                                "       \"Action\": \"s3:GetObject\",\n"
                                "       \"Principal\": \"*\",\n"
                                "       \"Resource\": \"*\"\n"
                                "   }\n"
                                "}\n");
    BOOST_CHECK_EQUAL(policy.size(), 1ull);

    {
        auto request = make_request("GET /my_object_id HTTP/1.1\r\n"
                                    "Host: bucket-id\r\n"
                                    "\r\n");

        auto result =
            policy.front().check(request, mock_command("s3:GetObject"));
        BOOST_CHECK(result.has_value());
        BOOST_CHECK(*result == effect::allow);
    }

    {
        auto request = make_request("GET /my_object_id HTTP/1.1\r\n"
                                    "Host: bucket-id\r\n"
                                    "\r\n");

        auto result =
            policy.front().check(request, mock_command("s3:PutObject"));
        BOOST_CHECK(!result.has_value());
    }
}

BOOST_AUTO_TEST_CASE(check_principal) {
    auto policy = parser::parse("{\n"
                                "   \"Version\": \"2012-10-17\",\n"
                                "   \"Statement\": {\n"
                                "       \"Sid\": \"AllowAnonForGetObject\",\n"
                                "       \"Effect\": \"Allow\",\n"
                                "       \"Action\": \"s3:GetObject\",\n"
                                "       \"Principal\": \"" +
                                user::ANONYMOUS_ARN +
                                "\",\n"
                                "       \"Resource\": \"*\"\n"
                                "   }\n"
                                "}\n");
    BOOST_CHECK_EQUAL(policy.size(), 1ull);

    {
        auto request =
            make_request("GET /bucket/my_object_id HTTP/1.1\r\n\r\n");
        auto result =
            policy.front().check(request, mock_command("s3:GetObject"));
        BOOST_CHECK(result.has_value());
        BOOST_CHECK(*result == effect::allow);
    }

    {
        auto request = make_request("GET /bucket/my_object_id HTTP/1.1\r\n\r\n",
                                    "arn:aws:iam::2:random_user");

        auto result =
            policy.front().check(request, mock_command("s3:GetObject"));
        BOOST_CHECK(!result.has_value());
    }
}

BOOST_AUTO_TEST_CASE(check_resource) {
    auto policy =
        parser::parse("{\n"
                      "   \"Version\": \"2012-10-17\",\n"
                      "   \"Statement\": {\n"
                      "       \"Sid\": \"AllowAllForResource\",\n"
                      "       \"Effect\": \"Allow\",\n"
                      "       \"Action\": \"s3:GetObject\",\n"
                      "       \"Principal\": \"*\",\n"
                      "       \"Resource\": \"arn:aws:s3:::bucket/*\"\n"
                      "   }\n"
                      "}\n");
    BOOST_CHECK_EQUAL(policy.size(), 1ull);

    {
        auto request =
            make_request("GET /bucket/my_object_id HTTP/1.1\r\n\r\n");
        auto result =
            policy.front().check(request, mock_command("s3:GetObject"));
        BOOST_CHECK(result.has_value());
        BOOST_CHECK(*result == effect::allow);
    }

    {
        auto request = make_request("GET /vedro/my_object_id HTTP/1.1\r\n\r\n");
        auto result =
            policy.front().check(request, mock_command("s3:GetObject"));
        BOOST_CHECK(!result.has_value());
    }
}
