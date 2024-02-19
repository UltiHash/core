#ifndef ENTRYPOINT_HTTP_COMPLETE_MULTIPART_H
#define ENTRYPOINT_HTTP_COMPLETE_MULTIPART_H

#include "entrypoint/http/http_request.h"
#include "entrypoint/http/http_response.h"
#include "entrypoint/utils/utils.h"

namespace uh::cluster {

class complete_multipart {
  public:
    explicit complete_multipart(entrypoint_state& entry_state);

    static bool can_handle(const http_request& req);

    coro<http_response> handle(const http_request& req) const;

  private:
    entrypoint_state& m_state;

    static void validate(const http_request& req);
};

} // namespace uh::cluster

#endif
