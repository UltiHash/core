#ifndef ENTRYPOINT_HTTP_LIST_OBJECTS_H
#define ENTRYPOINT_HTTP_LIST_OBJECTS_H

#include "entrypoint/common.h"
#include "http_request.h"
#include "http_response.h"
#include <utility>

namespace uh::cluster {

class list_objects {
  public:
    explicit list_objects(const entrypoint_state& entry_state);

    static bool can_handle(const http_request& req);

    coro<http_response> handle(const http_request& req) const;

  private:
    const entrypoint_state& m_state;
};

} // namespace uh::cluster

#endif
