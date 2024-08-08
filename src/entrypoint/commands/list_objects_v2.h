#ifndef UH_CLUSTER_LIST_OBJECTSV2_H
#define UH_CLUSTER_LIST_OBJECTSV2_H

#include "entrypoint/http/http_request.h"
#include "entrypoint/http/http_response.h"
#include "entrypoint/utils.h"

namespace uh::cluster {

class list_objects_v2 {

public:
    explicit list_objects_v2(const reference_collection& collection);

    static bool can_handle(const http_request& req);

    coro<http_response> handle(http_request& req) const;

private:
    const reference_collection& m_collection;
};

} // namespace uh::cluster

#endif // UH_CLUSTER_LIST_OBJECTSV2_H
