#ifndef ENTRYPOINT_HTTP_DELETE_OBJECT_H
#define ENTRYPOINT_HTTP_DELETE_OBJECT_H

#include "command.h"
#include "entrypoint/directory.h"
#include "entrypoint/limits.h"

namespace uh::cluster {

class delete_object : public command {
public:
    delete_object(directory&, limits&);

    static bool can_handle(const http_request& req);

    coro<http_response> handle(http_request& req) override;

private:
    directory& m_directory;
    limits& m_limits;
};

} // namespace uh::cluster

#endif
