#ifndef ENTRYPOINT_HTTP_COPY_OBJECT_H
#define ENTRYPOINT_HTTP_COPY_OBJECT_H

#include "command.h"
#include "entrypoint/directory.h"

namespace uh::cluster {

class copy_object : public command {
public:
    explicit copy_object(directory&);

    static bool can_handle(const http_request& req);

    coro<http_response> handle(http_request& req) override;

private:
    directory& m_directory;
};

} // namespace uh::cluster

#endif
