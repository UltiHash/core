#ifndef ENTRYPOINT_HTTP_LIST_MULTIPART_H
#define ENTRYPOINT_HTTP_LIST_MULTIPART_H

#include "entrypoint/http/http_request.h"
#include "entrypoint/http/http_response.h"
#include "entrypoint/utils/utils.h"

namespace uh::cluster {

class list_multipart {
  public:
    explicit list_multipart(const entrypoint_state& entry_state);

    static bool can_handle(const http_request& req);

    coro<http_response> handle(const http_request& req) const;

  private:
    const entrypoint_state& m_state;

    typedef struct {
        std::string upload_id;
        std::string object_name;
    } key_and_uploadid;

    static http_response
    get_response(const std::string& bucket_name,
                 const std::vector<key_and_uploadid>& ongoing) noexcept;
};

} // namespace uh::cluster

#endif
