#pragma once

#include <common/types/common_types.h>

#include <entrypoint/http/request.h>

#include <memory>

namespace uh::cluster::ep::http {

class no_auth {
public:
    static coro<std::unique_ptr<request>> create(partial_parse_result& req);
};

} // namespace uh::cluster::ep::http
