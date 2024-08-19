#ifndef ENTRYPOINT_COMMANDS_HEAD_OBJECT_H
#define ENTRYPOINT_COMMANDS_HEAD_OBJECT_H

#include "command.h"
#include "entrypoint/directory.h"

namespace uh::cluster {

struct head_object : public command {

    explicit head_object(directory&);

    static bool can_handle(const http_request& req);

    coro<http_response> handle(http_request& req) override;

private:
    directory& m_directory;
};

} // namespace uh::cluster

#endif
