#pragma once

#include "coordinator/service.h"

#include <common/etcd/utils.h>
#include <common/global_data/default_global_data_view.h>
#include <common/utils/temp_directory.h>
#include <config/configuration.h>
#include <storage/service.h>

namespace uh::cluster {
class global_data_view_fixture {
public:
    global_data_view_fixture()
        : m_etcd(),
          m_service_cfg(make_service_config()),
          m_storage_services(
              m_etcd,
              service_factory<storage_interface>(
                  m_ioc, m_gdv_config.storage_service_connection_count, {})) {}

    virtual ~global_data_view_fixture() { teardown(); }

    void setup() {

        for (size_t i = 0; i < NUM_STORAGE_INSTANCES; i++) {
            service_config service_cfg;
            service_cfg.working_dir = m_temp_dirs.emplace_back().path();
            storage_config storage_cfg;
            storage_cfg.server.port = 10000 + i;
            storage_cfg.m_data_store_roots = {
                std::filesystem::path(service_cfg.working_dir) / "storage"};
            m_storage_instances.emplace_back(
                std::make_unique<storage::service>(service_cfg, storage_cfg));
        }

        m_coordinator = std::make_unique<coordinator::service>(
            service_config{}, coordinator_config{});
        int i = 0;

        boost::asio::post(m_ioc, [this] { m_coordinator->run(); });
        m_threads.emplace_back([this, i] {
            try {
                m_ioc.run();
            } catch (std::exception& e) {
                m_excp_ptrs[i] = std::current_exception();
            }
        });
        i++;

        for (const auto& node : m_storage_instances) {
            boost::asio::post(m_ioc, [&node] { node->run(); });
            m_threads.emplace_back([this, i] {
                try {
                    m_ioc.run();
                } catch (std::exception& e) {
                    m_excp_ptrs[i] = std::current_exception();
                }
            });
            i++;
        }

        m_gdv = std::make_shared<default_global_data_view>(
            m_gdv_config, m_ioc, m_storage_services, m_etcd);

        m_threads.emplace_back([this, i] {
            try {
                m_ioc.run();
            } catch (std::exception& e) {
                m_excp_ptrs[i] = std::current_exception();
            }
        });

        wait_for_true(ETCD_TIMEOUT, std::chrono::seconds(1), [this]() {
            return m_storage_services.size() == m_storage_instances.size();
        });
    }

    void teardown() {
        for (const auto& node : m_storage_instances) {
            node->stop();
        }

        m_storage_instances.clear();

        if (m_coordinator) {
            m_coordinator->stop();
            m_coordinator.reset();
        }

        for (auto& thread : m_threads) {
            thread.join();
        }

        int i = 0;
        for (auto& exp : m_excp_ptrs) {
            if (exp) {
                LOG_ERROR() << "Exception in thread " << i;
                try {
                    std::rethrow_exception(exp);
                } catch (std::exception& e) {
                    throw e;
                }
            }
            i++;
        }

        m_threads.clear();
        m_temp_dirs.clear();
        std::string current_id_key =
            etcd_current_id_prefix_key + get_service_string(STORAGE_SERVICE);
        m_etcd.clear_all();
    }

    std::shared_ptr<global_data_view> get_global_data_view() { return m_gdv; }

    boost::asio::io_context& get_executor() { return m_ioc; }

private:
    service_config make_service_config() {
        service_config service_cfg;
        service_cfg.working_dir = m_temp_dirs.emplace_back().path();
        return service_cfg;
    }
    static constexpr size_t NUM_STORAGE_INSTANCES = 3;

    std::vector<std::exception_ptr> m_excp_ptrs{NUM_STORAGE_INSTANCES + 2};
    std::vector<temp_directory> m_temp_dirs;
    etcd_manager m_etcd;
    global_data_view_config m_gdv_config;
    service_config m_service_cfg;
    boost::asio::io_context m_ioc;
    std::vector<std::thread> m_threads;
    std::unique_ptr<coordinator::service> m_coordinator;
    std::vector<std::unique_ptr<storage::service>> m_storage_instances;
    service_maintainer<storage_interface> m_storage_services;
    std::shared_ptr<global_data_view> m_gdv;
};

} // namespace uh::cluster
