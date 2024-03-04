#ifndef UH_CLUSTER_GDV_FIXTURE_H
#define UH_CLUSTER_GDV_FIXTURE_H

#include <common/global_data/global_data_view.h>
#include <common/utils/temp_dir.h>
#include <config/config.h>
#include <storage/storage.h>

namespace uh::cluster {
class global_data_view_fixture {
public:
    global_data_view_fixture()
        : m_etcd_client("http://127.0.0.1:2379"),
          m_storage_services(m_ioc,
                             m_gdv_config.storage_service_connection_count,
                             m_etcd_client, m_gdv_config.max_data_store_size) {}

    void setup() {
        std::exception_ptr excp_ptr;

        for (size_t i = 0; i < NUM_STORAGE_INSTANCES; i++) {
            m_temp_dirs.emplace_back();
            service_config service_cfg;
            service_cfg.working_dir = m_temp_dirs.at(i).path();
            storage_config storage_cfg;
            storage_cfg.server.port = 10000 + i;
            storage_cfg.data_store.working_dir =
                service_cfg.working_dir / "storage";
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

        m_ioc.stop();
        m_ioc.restart();
    }

    std::shared_ptr<global_data_view> get_global_data_view() { return m_gdv; }

private:
    global_data_view_config m_gdv_config;
    boost::asio::io_context m_ioc;
    etcd::SyncClient m_etcd_client;
    services<STORAGE_SERVICE> m_storage_services;

    std::vector<temp_directory> m_temp_dirs;
    std::vector<std::unique_ptr<storage>> m_storage_instances;
    std::vector<std::thread> m_threads;

    std::shared_ptr<global_data_view> m_gdv;

    static constexpr size_t NUM_STORAGE_INSTANCES = 3;
};

} // namespace uh::cluster
#endif // UH_CLUSTER_GDV_FIXTURE_H
