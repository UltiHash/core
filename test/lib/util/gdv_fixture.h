#pragma once

#include "test_config.h"

#include <common/etcd/service.h>
#include <common/etcd/service_discovery/service_maintainer.h>
#include <common/etcd/utils.h>
#include <coordinator/service.h>
#include <storage/global/data_view.h>
#include <storage/service.h>

#include <util/temp_directory.h>

using namespace std::chrono_literals;

namespace uh::cluster {
class global_data_view_fixture {
public:
    global_data_view_fixture()
        : m_etcd(),
          m_service_cfg(make_service_config()) {}

    virtual ~global_data_view_fixture() { teardown(); }

    void setup() {
        {
            storage::group_configs configs;
            storage::group_config config;
            config.type = storage::group_config::type_t::ROUND_ROBIN;
            config.storages = NUM_STORAGE_INSTANCES;
            config.data_shards = 0;
            config.parity_shards = 0;
            configs.configs.clear();
            configs.configs.push_back(config);

            coordinator::service::publish_configs(m_etcd, configs);
        }
        try {
            for (size_t i = 0; i < NUM_STORAGE_INSTANCES; i++) {
                service_config service_cfg;
                service_cfg.working_dir = m_temp_dirs.emplace_back().path();
                storage_config storage_cfg;
                storage_cfg.server.port = 10000 + i;
                storage_cfg.m_data_store_roots = {
                    std::filesystem::path(service_cfg.working_dir) / "storage"};
                m_storage_instances.emplace_back(
                    std::make_unique<storage::service>(service_cfg,
                                                       storage_cfg));
            }
        } catch (...) {
            LOG_ERROR() << "Failed to create storage instances";
            throw;
        }

        int i = 0;

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

        m_gdv = std::make_unique<storage::global::global_data_view>(
            m_ioc, m_etcd, m_gdv_config);

        m_threads.emplace_back([this, i] {
            try {
                m_ioc.run();
            } catch (std::exception& e) {
                m_excp_ptrs[i] = std::current_exception();
            }
        });
    }

    void teardown() {
        m_gdv.reset();

        for (const auto& node : m_storage_instances) {
            node->stop();
        }

        m_storage_instances.clear();

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
        m_etcd.clear_all();
    }

    auto get_data_view() { return m_gdv; }

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

    std::vector<std::unique_ptr<storage::service>> m_storage_instances;

    std::shared_ptr<storage::global::global_data_view> m_gdv;
};

} // namespace uh::cluster
