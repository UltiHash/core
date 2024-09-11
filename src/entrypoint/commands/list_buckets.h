#ifndef ENTRYPOINT_HTTP_LIST_BUCKETS_H
#define ENTRYPOINT_HTTP_LIST_BUCKETS_H

#include "command.h"
#include "entrypoint/directory.h"

namespace uh::cluster {

class list_buckets : public command {
public:
    explicit list_buckets(directory&);

    static bool can_handle(const http_request& req);

    coro<http_response> handle(http_request& req) override;

    std::string action_id() const override;

private:
    directory& m_directory;
};

} // namespace uh::cluster

#endif
