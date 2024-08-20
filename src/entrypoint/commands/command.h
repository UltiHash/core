#ifndef COMMAND_H
#define COMMAND_H
#include "entrypoint/http/http_request.h"
#include "entrypoint/http/http_response.h"

namespace uh::cluster {

class command {
public:
    virtual coro<http_response> handle(http_request&) = 0;
    virtual coro<void> validate(const http_request& req) { co_return; }

    virtual ~command() = default;
};

} // end namespace uh::cluster

#endif // COMMAND_H
