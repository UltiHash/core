#ifndef ENTRYPOINT_HTTP_LIST_OBJECTS_H
#define ENTRYPOINT_HTTP_LIST_OBJECTS_H

#include "entrypoint/http/http_request.h"
#include "entrypoint/http/http_response.h"
#include "entrypoint/utils.h"

namespace uh::cluster {

class list_objects {
public:
    explicit list_objects(const reference_collection&);

    static bool can_handle(const http_request& req);

    coro<http_response> handle(http_request& req) const;

private:
    const reference_collection& m_collection;
};

} // namespace uh::cluster

#endif
