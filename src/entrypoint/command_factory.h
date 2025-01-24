#pragma once

#include "commands/command.h"
#include "common/global_data/global_data_view.h"
#include "common/service_interfaces/deduplicator_interface.h"
#include "config.h"
#include "directory.h"
#include "limits.h"
#include "multipart_state.h"

#include <entrypoint/user/db.h>

namespace uh::cluster {

struct command_factory {
    command_factory(boost::asio::io_context& ioc,
                    deduplicator_interface& dedupe, directory& dir,
                    multipart_state& uploads, entrypoint_config& config,
                    global_data_view& gdv, limits& uhlimits,
                    ep::user::db& users)
        : m_ioc(ioc),
          m_dedupe(dedupe),
          m_directory(dir),
          m_uploads(uploads),
          m_config(config),
          m_gdv(gdv),
          m_limits(uhlimits),
          m_users(users) {}

    coro<std::unique_ptr<command>> create(ep::http::request& req);

    [[nodiscard]] limits& get_limits() const;
    [[nodiscard]] directory& get_directory() const;

private:
    coro<std::unique_ptr<command>> action_command(ep::http::request& req);

    static constexpr std::size_t MAX_POST_QUERY_LENGTH = 64 * KIBI_BYTE;

    boost::asio::io_context& m_ioc;
    deduplicator_interface& m_dedupe;
    directory& m_directory;
    multipart_state& m_uploads;
    entrypoint_config& m_config;
    global_data_view& m_gdv;
    limits& m_limits;
    ep::user::db& m_users;
};

} // end namespace uh::cluster
