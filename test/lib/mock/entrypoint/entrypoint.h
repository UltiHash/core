#pragma once

#include <entrypoint/commands/command.h>
#include <entrypoint/policy/variables.h>

namespace uh::cluster::test {

class mock_command : public command {
public:
    mock_command(const std::string& id = "");
    coro<ep::http::response> handle(ep::http::request&) override;
    coro<void> validate(const ep::http::request& req) override;
    std::string action_id() const override;

private:
    std::string m_id;
};

class mock_body : public ep::http::body {
public:
    coro<std::span<const char>> read(std::size_t len) override;
    std::optional<std::size_t> length() const override;
    coro<void> consume() override;
    std::size_t buffer_size() const override;
};

ep::http::request
make_request(const std::string& code,
             const std::string& principal = ep::user::user::ANONYMOUS_ARN);

ep::policy::variables
vars(std::initializer_list<std::pair<std::string, std::string>> v);

} // namespace uh::cluster::test
