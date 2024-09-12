#ifndef COMMAND_FACTORY_H
#define COMMAND_FACTORY_H

#include "commands/command.h"
#include "common/etcd/service_discovery/roundrobin_load_balancer.h"
#include "common/global_data/global_data_view.h"
#include "common/service_interfaces/deduplicator_interface.h"
#include "config.h"
#include "directory.h"
#include "limits.h"
#include "multipart_state.h"

namespace uh::cluster {

struct command_factory {
    command_factory(
        boost::asio::io_context& ioc,
        roundrobin_load_balancer<deduplicator_interface>& dedupe_services,
        directory& dir, multipart_state& uploads, entrypoint_config& config,
        global_data_view& gdv, limits& uhlimits)
        : m_ioc(ioc),
          m_dedupe_services(dedupe_services),
          m_directory(dir),
          m_uploads(uploads),
          m_config(config),
          m_gdv(gdv),
          m_limits(uhlimits) {}

    [[nodiscard]] std::unique_ptr<command>
    create(const ep::http::request& req) const;

    [[nodiscard]] limits& get_limits() const;
    [[nodiscard]] directory& get_directory() const;

private:
    boost::asio::io_context& m_ioc;
    roundrobin_load_balancer<deduplicator_interface>& m_dedupe_services;
    directory& m_directory;
    multipart_state& m_uploads;
    entrypoint_config& m_config;
    global_data_view& m_gdv;
    limits& m_limits;
};

} // end namespace uh::cluster

#endif // COMMAND_FACTORY_H
