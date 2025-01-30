#pragma once

#include "command.h"
#include "common/service_interfaces/storage_interface.h"
#include "entrypoint/directory.h"
#include "entrypoint/limits.h"

namespace uh::cluster {

class copy_object : public command {
public:
    copy_object(directory&, storage_interface&, limits& limits);

    static bool can_handle(const ep::http::request& req);

    coro<ep::http::response> handle(ep::http::request& req) override;

    std::string action_id() const override;

private:
    directory& m_dir;
    storage_interface& m_gdv;
    limits& m_limits;
};

} // namespace uh::cluster
