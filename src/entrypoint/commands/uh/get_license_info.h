#pragma once

#include <common/license/license_watcher.h>
#include <entrypoint/commands/command.h>

namespace uh::cluster {

class get_license_info : public command {
public:
    get_license_info(license_watcher& w);

    static bool can_handle(const ep::http::request& req);

    coro<ep::http::response> handle(ep::http::request& req) override;

    std::string action_id() const override;

private:
    license_watcher& m_watcher;
};

} // namespace uh::cluster
