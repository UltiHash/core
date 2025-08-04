#include "forward_command.h"

namespace uh::cluster::proxy {

coro<ep::http::response> forward_command::handle(ep::http::request& req) {
    auto conn = m_pool.get();

    co_return conn->send_request(req);
}

coro<void> forward_command::validate(const ep::http::request& req) {
    co_return;
}

std::string forward_command::action_id() const { return "forward_command"; }

} // namespace uh::cluster::proxy
