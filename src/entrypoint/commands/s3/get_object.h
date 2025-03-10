#pragma once

#include "entrypoint/directory.h"
#include "storage/interface.h"
#include <entrypoint/commands/command.h>

namespace uh::cluster {

class get_object : public command {
public:
    get_object(directory&, sn::interface&);

    static bool can_handle(const ep::http::request& req);

    coro<ep::http::response> handle(ep::http::request& req) override;

    std::string action_id() const override;

private:
    directory& m_dir;
    sn::interface& m_storage;
};

} // namespace uh::cluster
