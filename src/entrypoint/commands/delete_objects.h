#ifndef ENTRYPOINT_HTTP_DELETE_OBJECTS_H
#define ENTRYPOINT_HTTP_DELETE_OBJECTS_H

#include "command.h"
#include "common/global_data/global_data_view.h"
#include "entrypoint/directory.h"
#include "entrypoint/limits.h"

namespace uh::cluster {

class delete_objects : public command {
public:
    delete_objects(directory&, global_data_view&, limits&);

    static bool can_handle(const ep::http::request& req);

    coro<ep::http::response> handle(ep::http::request& req) override;

    std::string action_id() const override;

private:
    directory& m_dir;
    global_data_view& m_gdv;
    limits& m_limits;
    static constexpr std::size_t MAXIMUM_DELETE_KEYS = 1000;
};

} // namespace uh::cluster

#endif
