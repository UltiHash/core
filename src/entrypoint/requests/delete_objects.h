#ifndef ENTRYPOINT_HTTP_DELETE_OBJECTS_H
#define ENTRYPOINT_HTTP_DELETE_OBJECTS_H

#include "entrypoint/http/http_request.h"
#include "entrypoint/http/http_response.h"
#include "entrypoint/rest/utils/parser/xml_parser.h"
#include "utils.h"

namespace uh::cluster {

class delete_objects {
  public:
    explicit delete_objects(const entrypoint_state& entry_state);

    static bool can_handle(const http_request& req);

    coro<http_response> handle(http_request& req) const;

  private:
    const entrypoint_state& m_state;

    static pugi::xpath_node_set validate(const http_request& req);

    struct fail {
        uint32_t code;
        std::string key;
    };

    static http_response
    get_response(const std::vector<std::string>& success,
                 const std::vector<fail>& failure) noexcept;
};

} // namespace uh::cluster

#endif
