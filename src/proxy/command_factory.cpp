#include "command_factory.h"

#include "forward_command.h"


namespace uh::cluster::proxy {

coro<std::unique_ptr<command>> command_factory::create(ep::http::request& req) {
    co_return std::unique_ptr<command>{};
}

} // namespace uh::cluster::proxy
