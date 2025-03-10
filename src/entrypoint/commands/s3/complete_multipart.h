#pragma once

#include "entrypoint/directory.h"
#include "entrypoint/limits.h"
#include "entrypoint/multipart_state.h"
#include "storage/interface.h"
#include <entrypoint/commands/command.h>

namespace uh::cluster {

class complete_multipart : public command {
public:
    complete_multipart(directory&, sn::interface&, multipart_state&, limits&);

    static bool can_handle(const ep::http::request& req);

    coro<ep::http::response> handle(ep::http::request& req) override;

    std::string action_id() const override;

private:
    directory& m_dir;
    multipart_state& m_uploads;
    limits& m_limits;
};

} // namespace uh::cluster
