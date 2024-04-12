#ifndef ENTRYPOINT_COMMANDS_HEAD_OBJECT_H
#define ENTRYPOINT_COMMANDS_HEAD_OBJECT_H

#include "entrypoint/http/http_request.h"
#include "entrypoint/http/http_response.h"
#include "entrypoint/utils.h"

namespace uh::cluster {

class head_object {
public:
    head_object(const reference_collection& coll);

    static bool can_handle(const http_request& req);

    coro<void> handle(const http_request& req) const;

private:
    const reference_collection& m_coll;
};

} // namespace uh::cluster

#endif
