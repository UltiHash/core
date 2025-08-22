#pragma once

#include <entrypoint/http/request_factory.h>

namespace uh::cluster::proxy {

class request_factory {
public:
    coro<std::unique_ptr<ep::http::request>> create(ep::http::stream& s,
                                                    ep::http::raw_request& req);
};

} // namespace uh::cluster::proxy
