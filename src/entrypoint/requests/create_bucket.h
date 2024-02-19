#ifndef ENTRYPOINT_HTTP_CREATE_BUCKET_H
#define ENTRYPOINT_HTTP_CREATE_BUCKET_H

#include "entrypoint/http/http_request.h"
#include "entrypoint/http/http_response.h"
#include "entrypoint/utils/utils.h"

namespace uh::cluster {

class create_bucket {
  public:
    explicit create_bucket(const entrypoint_state& entry_state);

    static bool can_handle(const http_request& req);

    coro<http_response> handle(const http_request& req) const;

  private:
    const entrypoint_state& m_state;
};

} // namespace uh::cluster

#endif
