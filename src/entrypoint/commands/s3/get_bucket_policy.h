#pragma once

#include "entrypoint/directory.h"
#include <entrypoint/commands/command.h>

namespace uh::cluster {

class get_bucket_policy : public command {
public:
    get_bucket_policy(directory& dir);

    static bool can_handle(const ep::http::request& req);

    coro<ep::http::response> handle(ep::http::request& req) override;

    std::string action_id() const override;

private:
    directory& m_dir;
};

} // namespace uh::cluster
