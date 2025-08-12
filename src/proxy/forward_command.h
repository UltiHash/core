#pragma once

#include <common/utils/pool.h>

#include <entrypoint/commands/command.h>

namespace uh::cluster::proxy {

class forward_command : public command {
public:
    forward_command(boost::beast::tcp_stream& to,
                    std::size_t buffer_size);

    coro<void> validate(const ep::http::request& req) override;
    coro<ep::http::response> handle(ep::http::request& r) override;
    std::string action_id() const override;

private:
    boost::beast::tcp_stream& m_to;
    std::size_t m_buffer_size;
    boost::beast::flat_buffer m_downstream_buffer;
    std::chrono::steady_clock::duration m_expected_timeout = std::chrono::seconds(1);
};

} // namespace uh::cluster::proxy
