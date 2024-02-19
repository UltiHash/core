#ifndef ENTRYPOINT_HTTP_LIST_OBJECTS_H
#define ENTRYPOINT_HTTP_LIST_OBJECTS_H

#include "entrypoint/http/http_request.h"
#include "entrypoint/http/http_response.h"
#include "utils.h"

namespace uh::cluster {

class list_objects {
  public:
    explicit list_objects(const entrypoint_state& entry_state);

    static bool can_handle(const http_request& req);

    coro<http_response> handle(const http_request& req) const;

  private:
    const entrypoint_state& m_state;

    static http_response get_response(const std::vector<std::string>& contents,
                                      const http_request& req);
};

} // namespace uh::cluster

#endif
