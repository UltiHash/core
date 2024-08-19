#ifndef ENTRYPOINT_HTTP_CREATE_BUCKET_H
#define ENTRYPOINT_HTTP_CREATE_BUCKET_H

#include "command.h"
#include "entrypoint/directory.h"

namespace uh::cluster {

struct create_bucket : public command {

    explicit create_bucket(directory&);

    static bool can_handle(const http_request& req);

    coro<http_response> handle(http_request& req) override;

private:
    directory& m_directory;
};

} // namespace uh::cluster

#endif
