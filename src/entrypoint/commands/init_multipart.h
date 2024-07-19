#ifndef UH_CLUSTER_INIT_MULTIPART_H
#define UH_CLUSTER_INIT_MULTIPART_H

#include "entrypoint/http/http_request.h"
#include "entrypoint/http/http_response.h"
#include "entrypoint/utils.h"

namespace uh::cluster {

class init_multipart {
public:
    explicit init_multipart(reference_collection& collection);

    static bool can_handle(const http_request& req);

    coro<http_response> handle(http_request& req);

private:
    reference_collection& m_collection;
};

} // namespace uh::cluster

#endif // UH_CLUSTER_INIT_MULTIPART_H
