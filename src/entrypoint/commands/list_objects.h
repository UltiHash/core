#ifndef ENTRYPOINT_HTTP_LIST_OBJECTS_H
#define ENTRYPOINT_HTTP_LIST_OBJECTS_H

#include "command.h"
#include "entrypoint/directory.h"

namespace uh::cluster {

class list_objects : public command {
public:
    explicit list_objects(directory&);

    static bool can_handle(const ep::http::request& req);

    coro<ep::http::response> handle(ep::http::request& req) override;

    std::string action_id() const override;

private:
    directory& m_dir;
};

} // namespace uh::cluster

#endif
