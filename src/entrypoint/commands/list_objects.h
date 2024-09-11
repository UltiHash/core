#ifndef ENTRYPOINT_HTTP_LIST_OBJECTS_H
#define ENTRYPOINT_HTTP_LIST_OBJECTS_H

#include "command.h"
#include "entrypoint/directory.h"

namespace uh::cluster {

class list_objects : public command {
public:
    explicit list_objects(directory&);

    static bool can_handle(const http_request& req);

    coro<http_response> handle(http_request& req) override;

    std::string action_id() const override;

private:
    directory& m_directory;
};

} // namespace uh::cluster

#endif
