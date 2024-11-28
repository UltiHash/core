#ifndef CORE_ENTRYPOINT_COMMANDS_HEAD_BUCKET_H
#define CORE_ENTRYPOINT_COMMANDS_HEAD_BUCKET_H

#include "command.h"
#include "entrypoint/directory.h"

namespace uh::cluster {

class head_bucket : public command {
public:
    explicit head_bucket(directory& dir);

    static bool can_handle(const ep::http::request& req);

    coro<ep::http::response> handle(ep::http::request& req) override;

    std::string action_id() const override;

private:
    directory& m_dir;
};

} // namespace uh::cluster

#endif
