#pragma once

#include "commands/command.h"
#include "common/service_interfaces/deduplicator_interface.h"
#include "config.h"
#include "directory.h"
#include "limits.h"
#include "multipart_state.h"
#include "storage/global/data_view.h"

#include <entrypoint/user/db.h>

namespace uh::cluster {

struct command_factory {
    command_factory(deduplicator_interface& dedupe, directory& dir,
                    multipart_state& uploads,
                    storage::global::global_data_view& gdv, limits& uhlimits,
                    ep::user::db& users, license_watcher& watcher)
        : m_dedupe(dedupe),
          m_directory(dir),
          m_uploads(uploads),
          m_gdv(gdv),
          m_limits(uhlimits),
          m_users(users),
          m_license_watcher(watcher) {}

    coro<std::unique_ptr<command>> create(ep::http::request& req);

    [[nodiscard]] limits& get_limits() const;
    [[nodiscard]] directory& get_directory() const;

private:
    coro<std::unique_ptr<command>> action_command(ep::http::request& req);

    static constexpr std::size_t MAX_POST_QUERY_LENGTH = 64 * KIBI_BYTE;

    deduplicator_interface& m_dedupe;
    directory& m_directory;
    multipart_state& m_uploads;
    storage::global::global_data_view& m_gdv;
    limits& m_limits;
    ep::user::db& m_users;
    license_watcher& m_license_watcher;
};

} // end namespace uh::cluster
