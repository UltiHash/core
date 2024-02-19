#ifndef ENTRYPOINT_HTTP_GET_BUCKET_H
#define ENTRYPOINT_HTTP_GET_BUCKET_H

#include "entrypoint/http/http_request.h"
#include "entrypoint/http/http_response.h"
#include "entrypoint/utils/utils.h"

namespace uh::cluster {

class get_bucket {
  public:
    explicit get_bucket(const entrypoint_state& entry_state);

    static bool can_handle(const http_request& req);

    coro<http_response> handle(const http_request& req) const;

  private:
    const entrypoint_state& m_state;

    static http_response get_response(const std::string& bucket_name) noexcept;
};

} // namespace uh::cluster

#endif
