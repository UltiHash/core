#pragma once

#include "command.h"
#include "common/service_interfaces/storage_interface.h"
#include "entrypoint/directory.h"
#include "entrypoint/limits.h"

namespace uh::cluster {

class delete_object : public command {
public:
    delete_object(directory&, storage_interface&, limits&);

    static bool can_handle(const ep::http::request& req);

    coro<ep::http::response> handle(ep::http::request& req) override;

    std::string action_id() const override;

private:
    directory& m_dir;
};

} // namespace uh::cluster
