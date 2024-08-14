#ifndef ENTRYPOINT_HTTP_DELETE_OBJECTS_H
#define ENTRYPOINT_HTTP_DELETE_OBJECTS_H

#include "entrypoint/http/http_request.h"
#include "entrypoint/http/http_response.h"
#include "entrypoint/utils.h"

namespace uh::cluster {

class delete_objects {
public:
    explicit delete_objects(const reference_collection&);

    static bool can_handle(const http_request& req);

    coro<http_response> handle(http_request& req) const;

private:
    const reference_collection& m_collection;
    static constexpr std::size_t MAXIMUM_DELETE_KEYS = 1000;
};

} // namespace uh::cluster

#endif
