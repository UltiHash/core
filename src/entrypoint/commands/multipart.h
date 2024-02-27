#ifndef ENTRYPOINT_HTTP_MULTIPART_H
#define ENTRYPOINT_HTTP_MULTIPART_H

#include "entrypoint/http/http_request.h"
#include "entrypoint/http/http_response.h"
#include "entrypoint/utils/utils.h"

namespace uh::cluster {

class multipart {
public:
    explicit multipart(reference_collection& collection);

    static bool can_handle(const http_request& req);

    coro<http_response> handle(http_request& req) const;

private:
    reference_collection& m_collection;
    md5 m_md5;
};

} // namespace uh::cluster

#endif
