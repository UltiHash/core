#ifndef ENTRYPOINT_HTTP_DELETE_BUCKET_H
#define ENTRYPOINT_HTTP_DELETE_BUCKET_H

#include "entrypoint/http/http_request.h"
#include "entrypoint/http/http_response.h"
#include "entrypoint/utils/utils.h"

namespace uh::cluster {

class delete_bucket {
public:
    explicit delete_bucket(const reference_collection&);

    static bool can_handle(const http_request& req);

    coro<http_response> handle(const http_request& req) const;

private:
    const reference_collection& m_collection;
};

} // namespace uh::cluster

#endif
