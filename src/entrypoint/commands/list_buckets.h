#ifndef ENTRYPOINT_HTTP_LIST_BUCKETS_H
#define ENTRYPOINT_HTTP_LIST_BUCKETS_H

#include "command.h"
#include "entrypoint/directory.h"

namespace uh::cluster {

struct list_buckets : public command {
    explicit list_buckets(directory&);

    static bool can_handle(const http_request& req);

    coro<http_response> handle(http_request& req) override;

private:
    directory& m_directory;
};

} // namespace uh::cluster

#endif
