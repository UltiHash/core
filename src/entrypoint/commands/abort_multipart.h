#ifndef ENTRYPOINT_HTTP_ABORT_MULTIPART_H
#define ENTRYPOINT_HTTP_ABORT_MULTIPART_H

#include "command.h"
#include "entrypoint/multipart_state.h"

namespace uh::cluster {

class abort_multipart : public command {
public:
    explicit abort_multipart(multipart_state&);

    static bool can_handle(const http_request& req);

    coro<http_response> handle(http_request& req) override;

    std::string action_id() const override;

private:
    multipart_state& m_uploads;
};

} // namespace uh::cluster

#endif
