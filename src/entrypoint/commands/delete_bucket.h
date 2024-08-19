#ifndef ENTRYPOINT_HTTP_DELETE_BUCKET_H
#define ENTRYPOINT_HTTP_DELETE_BUCKET_H

#include "command.h"
#include "entrypoint/directory.h"

namespace uh::cluster {

struct delete_bucket : public command {

    explicit delete_bucket(directory&);

    static bool can_handle(const http_request& req);

    coro<http_response> handle(http_request& req) override;

private:
    directory& m_directory;
};

} // namespace uh::cluster

#endif
