#include "entrypoint.h"

using namespace uh::cluster::ep::http;
using namespace uh::cluster::ep::policy;
using namespace uh::cluster::ep::user;

namespace uh::cluster::test {

mock_command::mock_command(const std::string& id)
    : m_id(id) {}

coro<response> mock_command::handle(request&) { co_return response{}; }

coro<void> mock_command::validate(const request& req) { co_return; }

std::string mock_command::action_id() const { return m_id; }

coro<std::size_t> mock_body::read(std::span<char>) { co_return 0ull; }

std::optional<std::size_t> mock_body::length() const { return {}; }

std::vector<boost::asio::const_buffer> mock_body::get_raw_buffer() const {
    throw std::runtime_error("not implemented");
}

ep::http::request make_request(const std::string& code,
                               const std::string& principal) {
    boost::beast::http::request_parser<boost::beast::http::empty_body> parser;
    boost::beast::error_code ec;

    parser.put(boost::asio::buffer(code), ec);

    return request(raw_request::from_string(parser.release(), {}, {}, {}),
                   std::make_unique<mock_body>(), user{.arn = principal});
}

variables vars(std::initializer_list<std::pair<std::string, std::string>> v) {
    static request req;
    static mock_command cmd;
    variables rv(req, cmd);

    for (auto& p : v) {
        rv.put(std::move(p.first), std::move(p.second));
    }

    return rv;
}

} // namespace uh::cluster::test
