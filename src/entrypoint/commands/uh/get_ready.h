#pragma once

#include <entrypoint/commands/command.h>
#include <entrypoint/directory.h>
#include <storage/interface.h>

namespace uh::cluster {

class get_ready : public command {
public:
    get_ready(directory&, sn::interface&);

    static bool can_handle(const ep::http::request& req);

    coro<ep::http::response> handle(ep::http::request& req) override;

    std::string action_id() const override;

private:
    directory& m_dir;
    sn::interface& m_gdv;
};

} // namespace uh::cluster
