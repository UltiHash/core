#pragma once

#include <entrypoint/directory.h>
#include <entrypoint/commands/command.h>

namespace uh::cluster {

class list_object_versions : public command {
public:
    list_object_versions(directory&);

    static bool can_handle(const ep::http::request& req);

    coro<ep::http::response> handle(ep::http::request& req) override;

    std::string action_id() const override;

private:
    directory& m_dir;
};

} // namespace uh::cluster
