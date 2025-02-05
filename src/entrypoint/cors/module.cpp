#include "module.h"

namespace uh::cluster::ep::cors {

module::module(directory& dir) :m_directory(dir) {}

coro<void> module::check(const http::request& request) const {
    std::optional<std::string> origin;
    if (origin = request.header("origin"); origin.has_value()) {

        // TODO should options be handled here?

        auto config = m_directory.get_bucket_cors(request.bucket());
        if (!config) {
            // TODO what to reply when there is no cors configuration for that
            // bucket?
            co_return;
        }

        auto infos = parser::parse(*config);
        if (auto it = infos.find(request.bucket()); it != infos.end()) {
            if (request.method() == http::verb::get && !it->allowed_get) {
                // TODO how to reply if GET requested but not allowed?
            }

            if (request.method() == http::verb::put && !it->allowed_put) {
                // TODO how to reply if GET requested but not allowed?
            }

            if (request.method() == http::verb::head && !it->allowed_head) {
                // TODO how to reply if GET requested but not allowed?
            }

            if (request.method() == http::verb::post && !it->allowed_post) {
                // TODO how to reply if GET requested but not allowed?
            }

            if (request.method() == http::verb::delete_ !it->allowed_delete) {
                // TODO how to reply if GET requested but not allowed?
            }
        }

        // TODO what to reply when there is no cors configuration for that
        // origin?

        /*if (!m_cors_origins.contains(*origin)) {
            throw command_exception(status::forbidden, "AccessDenied",
                                    "Access Denied");
        }*/
    }
    co_return;
}

} // namespace uh::cluster::ep::cors
