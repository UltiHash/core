#pragma once

#include "config.h"

#include <common/etcd/service.h>
#include <common/etcd/service_discovery/service_maintainer.h>
#include <common/telemetry/log.h>
#include <common/utils/common.h>
#include <common/utils/strings.h>
#include <storage/interfaces/remote_storage.h>

#include <boost/asio/spawn.hpp>
#include <boost/asio/steady_timer.hpp>

#include <common/license/backend_client.h>
#include <common/license/license_updater.h>
#include <common/license/usage_updater.h>
#include <ranges>

namespace uh::cluster::coordinator {

class service {
public:
    service(boost::asio::io_context& ioc, const service_config& service,
            const coordinator_config& cc)
        : m_etcd{service.etcd_config},
          m_usage{ioc, cc.database_config},
          m_license_updater{[&]() {
              if (cc.license) {
                  LOG_INFO() << "using license from UH_LICENSE";
                  return std::make_optional<license_updater>(
                      ioc, m_etcd,
                      pseudo_backend_client(cc.license.to_string()));
              } else {
                  LOG_INFO() << "using license from licensing host "
                             << cc.backend_config.backend_host;
                  const auto& bc = cc.backend_config;
                  return std::make_optional<license_updater>(
                      ioc, m_etcd,
                      default_backend_client(bc.backend_host, bc.customer_id,
                                             bc.access_token));
              }
          }()},
          m_usage_updater{[&]() {
              if (cc.license) {
                  return std::optional<usage_updater>(std::nullopt);
              } else {
                  const auto& bc = cc.backend_config;
                  return std::make_optional<usage_updater>(
                      ioc, m_usage, *m_license_updater,
                      default_backend_client(bc.backend_host, bc.customer_id,
                                             bc.access_token));
              }
          }()} {

        publish_configs(m_etcd, cc.storage_groups);
    }
    static void publish_configs(etcd_manager& etcd,
                                const storage::group_configs& group_configs) {
        for (const auto& cfg : group_configs.configs) {
            auto stored_config =
                etcd.get(ns::root.storage_groups.group_configs[cfg.id]);
            if (!stored_config.empty() and stored_config != cfg.to_string()) {
                throw std::runtime_error("supplied storage group configuration "
                                         "with id " +
                                         std::to_string(cfg.id) +
                                         " does not match with existing "
                                         "storage group configuration");
            }
            if (cfg.type != storage::group_config::type_t::ERASURE_CODING) {
                storage::group_config modified_config = cfg;
                modified_config.stripe_size_kib = DEFAULT_PAGE_SIZE / KIBI_BYTE;
                etcd.put(
                    ns::root.storage_groups.group_configs[modified_config.id],
                    modified_config.to_string());
            } else {
                etcd.put(ns::root.storage_groups.group_configs[cfg.id],
                         cfg.to_string());
            }
        }
    }

private:
    etcd_manager m_etcd;

    usage m_usage;
    std::optional<license_updater> m_license_updater;
    std::optional<usage_updater> m_usage_updater;
};
} // namespace uh::cluster::coordinator
