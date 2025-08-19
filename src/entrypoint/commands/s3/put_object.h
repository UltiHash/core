#pragma once

#include <common/service_interfaces/deduplicator_interface.h>
#include <entrypoint/directory.h>
#include <entrypoint/limits.h>
#include <storage/global/data_view.h>
#include <entrypoint/commands/command.h>

namespace uh::cluster {

class put_object : public command {
public:
    put_object(limits&, directory&, storage::global::global_data_view&,
               deduplicator_interface&);

    static bool can_handle(const ep::http::request& req);

    coro<void> validate(const ep::http::request& req) override;

    coro<ep::http::response> handle(ep::http::request& req) override;

    std::string action_id() const override;

private:
    directory& m_dir;
    storage::global::global_data_view& m_gdv;
    limits& m_limits;
    deduplicator_interface& m_dedupe;
};

} // namespace uh::cluster
