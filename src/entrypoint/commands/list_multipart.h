#ifndef ENTRYPOINT_HTTP_LIST_MULTIPART_H
#define ENTRYPOINT_HTTP_LIST_MULTIPART_H

#include "command.h"
#include "entrypoint/multipart_state.h"

namespace uh::cluster {

struct list_multipart : public command {
    explicit list_multipart(multipart_state&);

    static bool can_handle(const http_request& req);

    coro<http_response> handle(http_request& req) override;

private:
    multipart_state& m_uploads;
};

} // namespace uh::cluster

#endif
