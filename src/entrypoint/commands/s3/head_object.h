#pragma once

#include <entrypoint/directory.h>
#include <entrypoint/commands/command.h>

namespace uh::cluster {

class head_object : public command {
public:
    explicit head_object(directory&);

    static bool can_handle(const ep::http::request& req);

    coro<ep::http::response> handle(ep::http::request& req) override;

    std::string action_id() const override;

private:
    directory& m_dir;
};

} // namespace uh::cluster
