#ifndef ENTRYPOINT_HTTP_GET_OBJECT_H
#define ENTRYPOINT_HTTP_GET_OBJECT_H

#include "entrypoint/http/http_request.h"
#include "entrypoint/http/http_response.h"
#include "utils.h"

namespace uh::cluster {

class get_object {
  public:
    explicit get_object(const entrypoint_state& entry_state);

    static bool can_handle(const http_request& req);

    coro<http_response> handle(const http_request& req) const;

  private:
    const entrypoint_state& m_state;
};

} // namespace uh::cluster

#endif
