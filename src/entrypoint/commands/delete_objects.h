#ifndef ENTRYPOINT_HTTP_DELETE_OBJECTS_H
#define ENTRYPOINT_HTTP_DELETE_OBJECTS_H

#include "command.h"
#include "entrypoint/directory.h"
#include "entrypoint/limits.h"

namespace uh::cluster {

class delete_objects : public command {
public:
    delete_objects(directory&, limits&);

    static bool can_handle(const http_request& req);

    coro<http_response> handle(http_request& req) override;

private:
    directory& m_directory;
    limits& m_limits;
    static constexpr std::size_t MAXIMUM_DELETE_KEYS = 1000;
};

} // namespace uh::cluster

#endif
