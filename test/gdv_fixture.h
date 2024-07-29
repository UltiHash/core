#ifndef UH_CLUSTER_GDV_FIXTURE_H
#define UH_CLUSTER_GDV_FIXTURE_H

#include <common/global_data/global_data_view.h>
#include <common/utils/temp_directory.h>
#include <config/configuration.h>
#include <storage/storage.h>

namespace uh::cluster {
class global_data_view_fixture {
public:
    global_data_view_fixture()
        : m_etcd_client("http://127.0.0.1:2379"),
          m_service_cfg(make_service_config()),
          m_storage_services(
              m_etcd_client,
              service_factory<storage_interface>(
                  m_ioc, m_gdv_config.storage_service_connection_count, {})) {}

    ~global_data_view_fixture() { teardown(); }

    void setup() {
        std::exception_ptr excp_ptr;

        for (size_t i = 0; i < NUM_STORAGE_INSTANCES; i++) {
            service_config service_cfg;
            service_cfg.working_dir = m_temp_dirs.emplace_back().path();
            storage_config storage_cfg;
            storage_cfg.server.port = 10000 + i;
            storage_cfg.m_data_store_roots = {
                std::filesystem::path(service_cfg.working_dir) / "storage"};
            m_storage_instances.emplace_back(
                std::make_unique<storage>(service_cfg, storage_cfg));
        }

        for (const auto& node : m_storage_instances) {
            m_ioc.post([&node] { node->run(); });
            m_threads.emplace_back([&] {
                try {
                    m_ioc.run();
                } catch (std::exception& e) {
                    excp_ptr = std::current_exception();
                }
            });

            if (excp_ptr) {
                try {
                    std::rethrow_exception(excp_ptr);
                } catch (std::exception& e) {
                    teardown();
                    throw e;
                }
            }
        }

        m_gdv = std::make_shared<global_data_view>(m_gdv_config, m_ioc,
                                                   m_storage_services);

        m_threads.emplace_back([&] {
            try {
                m_ioc.run();
            } catch (std::exception& e) {
                excp_ptr = std::current_exception();
            }
        });

        if (excp_ptr) {
            try {
                std::rethrow_exception(excp_ptr);
            } catch (std::exception& e) {
                teardown();
                throw e;
            }
        }
    }

    void teardown() {
        for (const auto& node : m_storage_instances) {
            node->stop();
        }

        for (auto& thread : m_threads) {
            thread.join();
        }

        m_threads.clear();
        m_storage_instances.clear();
        m_temp_dirs.clear();
    }

    std::shared_ptr<global_data_view> get_global_data_view() { return m_gdv; }

private:
    service_config make_service_config() {
        service_config service_cfg;
        service_cfg.working_dir = m_temp_dirs.emplace_back().path();
        return service_cfg;
    }

    global_data_view_config m_gdv_config;
    boost::asio::io_context m_ioc;

    etcd::SyncClient m_etcd_client;
    std::vector<temp_directory> m_temp_dirs;
    service_config m_service_cfg;
    services<storage_interface> m_storage_services;

    std::vector<std::unique_ptr<storage>> m_storage_instances;
    std::vector<std::thread> m_threads;

    std::shared_ptr<global_data_view> m_gdv;

    static constexpr size_t NUM_STORAGE_INSTANCES = 3;
};

} // namespace uh::cluster
#endif // UH_CLUSTER_GDV_FIXTURE_H
