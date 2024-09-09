#ifndef ENTRYPOINT_HTTP_DELETE_BUCKET_H
#define ENTRYPOINT_HTTP_DELETE_BUCKET_H

#include "command.h"
#include "entrypoint/directory.h"

namespace uh::cluster {

class delete_bucket : public command {
public:
    explicit delete_bucket(directory&);

    static bool can_handle(const http_request& req);

    coro<http_response> handle(http_request& req) override;

    std::string action_id() const override;

private:
    directory& m_directory;
};

} // namespace uh::cluster

#endif
