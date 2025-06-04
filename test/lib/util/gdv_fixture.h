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
#if defined(WITH_EC)
    global_data_view_fixture(
        const storage::group_config& config =
            {
                .id = 0,
                .type = storage::group_config::type_t::ERASURE_CODING,
                .storages = 3,
                .data_shards = 2,
                .parity_shards = 1,
                .stripe_size_kib = 2 * 2,
            })
#else
    global_data_view_fixture(
        const storage::group_config& config =
            {
                .id = 0,
                .type = storage::group_config::type_t::ROUND_ROBIN,
                .storages = 3,
            })
#endif
        : m_config{config},
          m_etcd(),
          m_work_guard(boost::asio::make_work_guard(m_ioc)) {
    }

    virtual ~global_data_view_fixture() {}

    void setup() {

        m_etcd.clear_all();
        std::this_thread::sleep_for(100ms);

        if (m_config.type == storage::group_config::type_t::ERASURE_CODING) {
            if (m_config.storages !=
                    m_config.data_shards + m_config.parity_shards or
                m_config.stripe_size_kib % m_config.data_shards != 0) {
                throw std::runtime_error("Invalid group config");
            }
        }

        // NOTE: Now support only one group
        storage::group_configs configs;
        configs.configs.push_back(m_config);
        m_storage_instances.resize(m_config.storages);
        coordinator::service::publish_configs(m_etcd, configs);
        try {
            for (size_t i = 0; i < m_config.storages; i++) {
                m_temp_dirs.emplace_back(std::make_unique<temp_directory>());
                activate_storage(i);
            }
        } catch (...) {
            LOG_ERROR() << "Failed to create storage instances";
            throw;
        }

        m_thread = std::thread([this] {
            try {
                m_ioc.run();
            } catch (std::exception& e) {
                m_exception_ptr = std::current_exception();
            }
        });

        m_gdv = std::make_unique<storage::global::global_data_view>(
            m_ioc, m_etcd, m_gdv_config);

        std::this_thread::sleep_for(100ms);
    }

    void teardown() {
        m_gdv.reset();

        for (const auto& node : m_storage_instances) {
            if (node != nullptr)
                node->stop();
        }

        m_storage_instances.clear();

        m_work_guard.reset();

        m_thread.join();

        if (m_exception_ptr) {
            try {
                std::rethrow_exception(m_exception_ptr);
            } catch (std::exception& e) {
                throw e;
            }
        }

        m_temp_dirs.clear();
        m_etcd.clear_all();
    }
    etcd_manager& get_etcd_manager() { return m_etcd; }

    auto get_group_config() { return m_config; }

    void deactivate_storage(std::size_t id) {
        auto& node = m_storage_instances.at(id);
        if (node != nullptr) {
            node->stop();
            node.reset();
        }
    }

    void activate_storage(std::size_t id, std::size_t port_offset = 10000) {
        service_config service_cfg;
        service_cfg.working_dir = m_temp_dirs[id]->path();
        storage_config storage_cfg;
        storage_cfg.server.port = port_offset + id;
        storage_cfg.working_directory = {
            std::filesystem::path(service_cfg.working_dir) / "storage"};
        LOG_DEBUG() << "storage " << id
                    << " path: " << storage_cfg.working_directory;
        storage_cfg.instance_id = id;
        storage_cfg.group_id = m_config.id;
        m_storage_instances[id] =
            std::make_unique<storage::service>(service_cfg, storage_cfg);
    }

    void introduce_new_storage(std::size_t id,
                               std::size_t port_offset = 10000) {
        m_temp_dirs[id] = std::make_unique<temp_directory>();
        activate_storage(id, port_offset);
    }

    auto get_data_view() { return m_gdv; }

    boost::asio::io_context& get_executor() { return m_ioc; }

private:
    storage::group_config m_config;

    static constexpr size_t NUM_STORAGE_INSTANCES = 3;

    std::exception_ptr m_exception_ptr;
    std::vector<std::unique_ptr<temp_directory>> m_temp_dirs;
    etcd_manager m_etcd;
    global_data_view_config m_gdv_config;
    boost::asio::io_context m_ioc;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type>
        m_work_guard;
    std::thread m_thread;

    std::vector<std::unique_ptr<storage::service>> m_storage_instances;

    std::shared_ptr<storage::global::global_data_view> m_gdv;
};

} // namespace uh::cluster
