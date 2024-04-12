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

    coro<void> handle(http_request& req);

    static http_response get_response(const std::vector<object>& objects,
                                      const http_request& req);

private:
    const reference_collection& m_collection;
};

} // namespace uh::cluster

#endif // UH_CLUSTER_LIST_OBJECTSV2_H
