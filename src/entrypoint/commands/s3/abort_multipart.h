#pragma once

#include "entrypoint/directory.h"
#include "entrypoint/multipart_state.h"
#include "storage/interface.h"
#include <entrypoint/commands/command.h>

namespace uh::cluster {

class abort_multipart : public command {
public:
    abort_multipart(multipart_state&, sn::interface&);

    static bool can_handle(const ep::http::request& req);

    coro<ep::http::response> handle(ep::http::request& req) override;

    std::string action_id() const override;

private:
    multipart_state& m_uploads;
    sn::interface& m_gdv;
};

} // namespace uh::cluster
