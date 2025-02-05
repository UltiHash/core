#include "module.h"

namespace uh::cluster::ep::cors {

module::module(directory& dir) :m_directory(dir) {}

coro<void> module::check(const http::request& request) const {
    std::optional<std::string> origin;
    if (origin = request.header("origin"); origin.has_value()) {
        (void)m_directory;
        /*if (!m_cors_origins.contains(*origin)) {
            throw command_exception(status::forbidden, "AccessDenied",
                                    "Access Denied");
        }*/
    }
    co_return;
}

} // namespace uh::cluster::ep::cors
