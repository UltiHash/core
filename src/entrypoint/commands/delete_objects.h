#pragma once

#include "command.h"
#include "entrypoint/directory.h"

namespace uh::cluster {

class delete_objects : public command {
public:
    delete_objects(directory&);

    static bool can_handle(const ep::http::request& req);

    coro<ep::http::response> handle(ep::http::request& req) override;

    std::string action_id() const override;

private:
    directory& m_dir;
    static constexpr std::size_t MAXIMUM_DELETE_KEYS = 1000;
};

} // namespace uh::cluster
