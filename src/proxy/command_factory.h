#pragma once

#include <common/utils/pool.h>

#include <entrypoint/command_factory.h>


namespace uh::cluster::proxy {

class command_factory {
public:
    coro<std::unique_ptr<command>> create(ep::http::request& req);
};

} // namespace uh::cluster::proxy
