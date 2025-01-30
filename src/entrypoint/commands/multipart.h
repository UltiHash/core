#pragma once

#include "command.h"
#include "common/service_interfaces/deduplicator_interface.h"
#include "common/service_interfaces/storage_interface.h"
#include "entrypoint/directory.h"
#include "entrypoint/multipart_state.h"

namespace uh::cluster {

class multipart : public command {
public:
    multipart(deduplicator_interface&, storage_interface&, multipart_state&);

    static bool can_handle(const ep::http::request& req);

    coro<void> validate(const ep::http::request& req) override;

    coro<ep::http::response> handle(ep::http::request& req) override;

    std::string action_id() const override;

private:
    deduplicator_interface& m_dedupe;
    storage_interface& m_gdv;
    multipart_state& m_uploads;
};

} // namespace uh::cluster
