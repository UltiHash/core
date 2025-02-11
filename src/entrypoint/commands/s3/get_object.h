#pragma once

#include "common/global_data/global_data_view.h"
#include "entrypoint/directory.h"
#include <entrypoint/commands/command.h>

namespace uh::cluster {

class get_object : public command {
public:
    get_object(directory&, global_data_view&);

    static bool can_handle(const ep::http::request& req);

    coro<ep::http::response> handle(ep::http::request& req) override;

    std::string action_id() const override;

private:
    directory& m_dir;
    global_data_view& m_storage;
};

} // namespace uh::cluster
