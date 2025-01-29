#pragma once

#include "command.h"
#include "common/service_interfaces/storage_interface.h"
#include "entrypoint/directory.h"

namespace uh::cluster {

class get_object : public command {
public:
    get_object(directory&, storage_interface&);

    static bool can_handle(const ep::http::request& req);

    coro<ep::http::response> handle(ep::http::request& req) override;

    std::string action_id() const override;

private:
    directory& m_dir;
    global_data_view& m_storage;
};

} // namespace uh::cluster
