#pragma once

#include "entrypoint/directory.h"
#include "entrypoint/multipart_state.h"
#include <entrypoint/commands/command.h>

namespace uh::cluster {

class init_multipart : public command {
public:
    explicit init_multipart(directory&, multipart_state&);

    static bool can_handle(const ep::http::request& req);

    coro<ep::http::response> handle(ep::http::request& req) override;

    std::string action_id() const override;

private:
    directory& m_dir;
    multipart_state& m_uploads;
};

} // namespace uh::cluster
