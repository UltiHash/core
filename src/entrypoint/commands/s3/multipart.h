#pragma once

#include "common/global_data/global_data_view.h"
#include "common/service_interfaces/deduplicator_interface.h"
#include "entrypoint/directory.h"
#include "entrypoint/multipart_state.h"
#include <entrypoint/commands/command.h>

namespace uh::cluster {

class multipart : public command {
public:
    multipart(deduplicator_interface&, global_data_view&, multipart_state&);

    static bool can_handle(const ep::http::request& req);

    coro<void> validate(const ep::http::request& req) override;

    coro<ep::http::response> handle(ep::http::request& req) override;

    std::string action_id() const override;

private:
    deduplicator_interface& m_dedupe;
    global_data_view& m_gdv;
    multipart_state& m_uploads;
};

} // namespace uh::cluster
