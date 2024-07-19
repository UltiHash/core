#ifndef ENTRYPOINT_HTTP_GET_STATISTICS_H
#define ENTRYPOINT_HTTP_GET_STATISTICS_H

#include "entrypoint/http/http_request.h"
#include "entrypoint/http/http_response.h"
#include "entrypoint/utils.h"

namespace uh::cluster {

class get_metrics {
public:
    explicit get_metrics(const reference_collection&);

    static bool can_handle(const http_request& req);

    coro<http_response> handle(http_request& req) const;

private:
    const reference_collection& m_collection;
};

} // namespace uh::cluster

#endif
