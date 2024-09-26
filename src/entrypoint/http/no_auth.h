#ifndef CORE_ENTRYPOINT_HTTP_NO_AUTH_H
#define CORE_ENTRYPOINT_HTTP_NO_AUTH_H

#include <common/types/common_types.h>

#include <entrypoint/http/beast_utils.h>
#include <entrypoint/http/request.h>

#include <memory>

namespace uh::cluster::ep::http {

class no_auth {
public:
    static coro<std::unique_ptr<request>> create(partial_parse_result& req);
};

} // namespace uh::cluster::ep::http

#endif
