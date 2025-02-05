#pragma once

#include <entrypoint/directory.h>
#include <entrypoint/http/request.h>

namespace uh::cluster::ep::cors {

class module {
public:
    module(directory& dir);

    coro<void> check(const http::request& request) const;

private:
    directory& m_directory;
};

} // namespace uh::cluster::ep::cors
