#pragma once

#include "entrypoint/directory.h"
#include "entrypoint/limits.h"
#include "storage/global/data_view.h"
#include <entrypoint/commands/command.h>

namespace uh::cluster {

class delete_object : public command {
public:
    delete_object(directory&, storage::global::global_data_view&, limits&);

    static bool can_handle(const ep::http::request& req);

    coro<ep::http::response> handle(ep::http::request& req) override;

    std::string action_id() const override;

private:
    directory& m_dir;
};

} // namespace uh::cluster
