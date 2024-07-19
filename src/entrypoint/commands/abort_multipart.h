#ifndef ENTRYPOINT_HTTP_ABORT_MULTIPART_H
#define ENTRYPOINT_HTTP_ABORT_MULTIPART_H

#include "entrypoint/http/http_request.h"
#include "entrypoint/http/http_response.h"
#include "entrypoint/utils.h"

namespace uh::cluster {

class abort_multipart {
public:
    explicit abort_multipart(reference_collection&);

    static bool can_handle(const http_request& req);

    coro<http_response> handle(http_request& req) const;

private:
    reference_collection& m_collection;
};

} // namespace uh::cluster

#endif
