#pragma once

#include "entrypoint/directory.h"
#include "entrypoint/limits.h"
#include "storage/interface.h"
#include <entrypoint/commands/command.h>

namespace uh::cluster {

class copy_object : public command {
public:
    copy_object(directory&, sn::interface&, limits& limits);

    static bool can_handle(const ep::http::request& req);

    coro<ep::http::response> handle(ep::http::request& req) override;

    std::string action_id() const override;

private:
    directory& m_dir;
    sn::interface& m_gdv;
    limits& m_limits;
};

} // namespace uh::cluster
