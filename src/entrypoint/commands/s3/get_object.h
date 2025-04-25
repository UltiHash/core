#pragma once

#include <storage/global/data_view.h>

#include <entrypoint/commands/command.h>
#include <entrypoint/directory.h>

namespace uh::cluster {

class get_object : public command {
public:
    get_object(directory&, storage::global::global_data_view&);

    static bool can_handle(const ep::http::request& req);

    coro<ep::http::response> handle(ep::http::request& req) override;

    std::string action_id() const override;

private:
    directory& m_dir;
    storage::global::global_data_view& m_storage;
};

} // namespace uh::cluster
