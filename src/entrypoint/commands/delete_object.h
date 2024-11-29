#ifndef ENTRYPOINT_HTTP_DELETE_OBJECT_H
#define ENTRYPOINT_HTTP_DELETE_OBJECT_H

#include "command.h"
#include "common/global_data/global_data_view.h"
#include "entrypoint/directory.h"
#include "entrypoint/limits.h"

namespace uh::cluster {

class delete_object : public command {
public:
    delete_object(directory&, global_data_view&, limits&);

    static bool can_handle(const ep::http::request& req);

    coro<ep::http::response> handle(ep::http::request& req) override;

    std::string action_id() const override;

private:
    directory& m_dir;
};

} // namespace uh::cluster

#endif
