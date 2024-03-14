#ifndef ENTRYPOINT_HTTP_PUT_OBJECT_H
#define ENTRYPOINT_HTTP_PUT_OBJECT_H

#include "common/utils/worker_pool.h"
#include "entrypoint/http/command_exception.h"
#include "entrypoint/http/http_request.h"
#include "entrypoint/http/http_response.h"
#include "entrypoint/utils.h"

namespace uh::cluster {

class put_object {
public:
    explicit put_object(const reference_collection&);

    static bool can_handle(const http_request& req);

    coro<void> handle(http_request& req) const;

private:
    const reference_collection& m_collection;
};

} // namespace uh::cluster

#endif
