#pragma once

#include "entrypoint/directory.h"
#include "entrypoint/multipart_state.h"
#include "storage/global/data_view.h"
#include <entrypoint/commands/command.h>

namespace uh::cluster {

class abort_multipart : public command {
public:
    abort_multipart(multipart_state&, storage::global::global_data_view&);

    static bool can_handle(const ep::http::request& req);

    coro<ep::http::response> handle(ep::http::request& req) override;

    std::string action_id() const override;

private:
    multipart_state& m_uploads;
    storage::global::global_data_view& m_gdv;
};

} // namespace uh::cluster
