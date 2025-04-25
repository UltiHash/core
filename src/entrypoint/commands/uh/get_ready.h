#pragma once

#include <entrypoint/commands/command.h>
#include <entrypoint/directory.h>
#include <storage/global/data_view.h>

namespace uh::cluster {

class get_ready : public command {
public:
    get_ready(directory&, storage::global::global_data_view&);

    static bool can_handle(const ep::http::request& req);

    coro<ep::http::response> handle(ep::http::request& req) override;

    std::string action_id() const override;

private:
    directory& m_dir;
    storage::global::global_data_view& m_gdv;
};

} // namespace uh::cluster
