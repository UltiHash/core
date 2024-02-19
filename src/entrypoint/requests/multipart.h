#ifndef ENTRYPOINT_HTTP_MULTIPART_H
#define ENTRYPOINT_HTTP_MULTIPART_H

#include "entrypoint/http/http_request.h"
#include "entrypoint/http/http_response.h"
#include "utils.h"

namespace uh::cluster {

class multipart {
  public:
    explicit multipart(entrypoint_state& entry_state);

    static bool can_handle(const http_request& req);

    coro<http_response> handle(http_request& req) const;

  private:
    entrypoint_state& m_state;

    static void validate(const http_request& req);
};

} // namespace uh::cluster

#endif
