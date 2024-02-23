#ifndef ENTRYPOINT_HTTP_PUT_OBJECT_H
#define ENTRYPOINT_HTTP_PUT_OBJECT_H

#include "common/utils/worker_utils.h"
#include "entrypoint/http/custom_error_response_exception.h"
#include "entrypoint/http/http_request.h"
#include "entrypoint/http/http_response.h"
#include "entrypoint/utils/utils.h"

namespace uh::cluster {

class put_object {
public:
    explicit put_object(const entrypoint_state& entry_state);

    static bool can_handle(const http_request& req);

    coro<http_response> handle(http_request& req) const;

private:
    const entrypoint_state& m_state;
};

} // namespace uh::cluster

#endif
