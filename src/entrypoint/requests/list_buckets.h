#ifndef ENTRYPOINT_HTTP_LIST_BUCKETS_H
#define ENTRYPOINT_HTTP_LIST_BUCKETS_H

#include "entrypoint/http/http_request.h"
#include "entrypoint/http/http_response.h"
#include "utils.h"
#include <utility>

namespace uh::cluster {

class list_buckets {
  public:
    explicit list_buckets(const entrypoint_state& entry_state);

    static bool can_handle(const http_request& req);

    coro<http_response> handle(const http_request& req) const;

  private:
    const entrypoint_state& m_state;

    static http_response
    get_response(const std::vector<std::string>& buckets_found) noexcept;
};

} // namespace uh::cluster

#endif
