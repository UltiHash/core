#pragma once

#include "test_config.h"

#include <boost/process.hpp>
#include <common/etcd/service.h>
#include <common/etcd/service_discovery/service_maintainer.h>
#include <common/etcd/utils.h>
#include <coordinator/service.h>
#include <storage/global/data_view.h>
#include <storage/service.h>

#include <util/temp_directory.h>

namespace bp = boost::process;
using namespace std::chrono_literals;

namespace uh::cluster {

void send_signal_to_process(pid_t pid, int signal) {
    if (kill(pid, signal) == 0) {
        std::cout << "Signal " << signal << " sent to process " << pid << ".\n";
    } else {
        perror("Failed to send signal");
    }
}

class gdv_fixture_using_process {
public:
#if defined(WITH_EC)
    gdv_fixture_using_process(
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
    gdv_fixture_using_process(
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

    virtual ~gdv_fixture_using_process() {}

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
            if (node != nullptr) {
                node->terminate();
            }
        }

        for (auto& node : m_storage_instances) {
            if (node != nullptr) {
                node->wait();
                node.reset();
            }
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
            send_signal_to_process(node->id(), SIGTERM);
            node->wait();
            node.reset();
        }
    }

    void kill_storage(std::size_t id) {
        auto& node = m_storage_instances.at(id);
        if (node != nullptr) {
            // node->terminate();
            send_signal_to_process(node->id(), SIGKILL);
            node->wait();
            node.reset();
        }
    }

    void activate_storage(std::size_t id, std::size_t port_offset = 10000) {
        bp::environment env = boost::this_process::environment();
        env[ENV_CFG_LICENSE] = test_license_string;
        env[ENV_CFG_LOG_LEVEL] = "DEBUG";
        env["OTEL_RESOURCE_ATTRIBUTES"] = "service.name=storage";
        env[ENV_CFG_STORAGE_GROUP_ID] = serialize(m_config.id);
        env[ENV_CFG_STORAGE_SERVICE_ID] = serialize(id);

        std::vector<std::string> args = {"--workdir", m_temp_dirs[id]->path(),
                                         "storage", //
                                         "--port", serialize(port_offset + id)};

        m_storage_instances[id] = std::make_unique<bp::child>(
            UH_CLUSTER_EXECUTABLE, bp::env = env, bp::args = args);

        LOG_DEBUG() << "storage " << id << " is spawned with process id: "
                    << m_storage_instances[id]->id();
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

    std::vector<std::unique_ptr<bp::child>> m_storage_instances;

    std::shared_ptr<storage::global::global_data_view> m_gdv;
};

} // namespace uh::cluster
