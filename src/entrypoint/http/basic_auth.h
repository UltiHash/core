#pragma once

#include <entrypoint/http/request.h>
#include <entrypoint/user/db.h>

namespace uh::cluster::ep::http {

class basic_auth {
public:
    static coro<std::unique_ptr<request>>
    create(stream& s, user::db& users, raw_request req);
};

} // namespace uh::cluster::ep::http
