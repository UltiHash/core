#ifndef CORE_ENTRYPOINT_HTTP_BASIC_AUTH_H
#define CORE_ENTRYPOINT_HTTP_BASIC_AUTH_H

#include "entrypoint/http/user_db.h"
#include <entrypoint/http/request.h>

namespace uh::cluster::ep::http {

class basic_auth {
public:
    static coro<std::unique_ptr<request>> create(user::db& users,
                                                 partial_parse_result& req);
};

} // namespace uh::cluster::ep::http

#endif
