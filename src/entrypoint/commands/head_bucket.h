#ifndef CORE_ENTRYPOINT_COMMANDS_HEAD_BUCKET_H
#define CORE_ENTRYPOINT_COMMANDS_HEAD_BUCKET_H

#include "command.h"
#include "entrypoint/directory.h"

namespace uh::cluster {

class head_bucket : public command {
public:
    explicit head_bucket(directory& dir);

    static bool can_handle(const http_request& req);

    coro<http_response> handle(http_request& req) override;

    std::string action_id() const override;

private:
    directory& m_directory;
};

} // namespace uh::cluster

#endif
