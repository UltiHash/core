#ifndef UH_CLUSTER_CONFIG_REGISTRY_H
#define UH_CLUSTER_CONFIG_REGISTRY_H

#include "common/telemetry/log.h"
#include "common/utils/cluster_config.h"
#include "common/utils/common.h"
#include "common/utils/time_utils.h"
#include "etcd/SyncClient.hpp"
#include "namespace.h"
#include <fstream>
#include <string>

namespace uh::cluster {

class config_registry {

public:
    config_registry(uh::cluster::role role, etcd::SyncClient& etcd_client,
                    const std::filesystem::path& working_dir,
                    std::size_t service_id)
        : m_etcd_client(etcd_client),
          m_service_role(role),
          m_working_dir(working_dir / get_service_string(m_service_role)),
          m_service_name(get_service_string(m_service_role) + "/" +
                         std::to_string(service_id)) {
        init_default_config_values();
    }

    server_config get_server_config() {
        return {.threads = get_config_value_ull(CFG_SERVER_THREADS),
                .port = static_cast<uint16_t>(
                    get_config_value_ull(CFG_SERVER_PORT)),
                .bind_address = get_config_value_string(CFG_SERVER_BIND_ADDR)};
    }

    global_data_view_config get_global_data_view_config() {
        if (m_service_role != uh::cluster::DEDUPLICATOR_SERVICE and
            m_service_role != uh::cluster::DIRECTORY_SERVICE)
            throw std::invalid_argument(
                "Only service instances of the type '" +
                get_service_string(uh::cluster::DEDUPLICATOR_SERVICE) +
                "' or '" + get_service_string(uh::cluster::DIRECTORY_SERVICE) +
                "' may access the global data view configuration!");
        return {
            .storage_service_connection_count =
                get_config_value_ull(CFG_GDV_STORAGE_SERVICE_CONNECTION_COUNT),
            .read_cache_capacity_l1 =
                get_config_value_ull(CFG_GDV_READ_CACHE_CAPACITY_L1),
            .read_cache_capacity_l2 =
                get_config_value_ull(CFG_GDV_READ_CACHE_CAPACITY_L2),
            .l1_sample_size = get_config_value_ull(CFG_GDV_L1_SAMPLE_SIZE),
            .max_data_store_size =
                get_config_value_ull(CFG_GDV_MAX_DATA_STORE_SIZE),
        };
    }

    deduplicator_config get_deduplicator_config() {
        if (m_service_role != uh::cluster::DEDUPLICATOR_SERVICE)
            throw std::invalid_argument(
                "Only service instances of the type '" +
                get_service_string(uh::cluster::DEDUPLICATOR_SERVICE) +
                "' may access the " +
                get_service_string(uh::cluster::DEDUPLICATOR_SERVICE) +
                " configuration!");
        return {
            .working_dir = m_working_dir,
            .min_fragment_size =
                get_config_value_ull(CFG_DEDUP_MIN_FRAGMENT_SIZE),
            .max_fragment_size =
                get_config_value_ull(CFG_DEDUP_MAX_FRAGMENT_SIZE),
            .dedupe_worker_minimum_data_size =
                get_config_value_ull(CFG_DEDUP_WORKER_MIN_DATA_SIZE),
            .worker_thread_count =
                get_config_value_ull(CFG_DEDUP_WORKER_THREAD_COUNT),
        };
    }

    directory_config get_directory_config() {
        if (m_service_role != uh::cluster::DIRECTORY_SERVICE)
            throw std::invalid_argument(
                "Only service instances of the type '" +
                get_service_string(uh::cluster::DIRECTORY_SERVICE) +
                "' may access the " +
                get_service_string(uh::cluster::DIRECTORY_SERVICE) +
                " configuration!");
        return {
            .directory_store_conf =
                {
                    .working_dir = m_working_dir,
                    .bucket_conf =
                        {
                            .min_file_size =
                                get_config_value_ull(CFG_DIR_MIN_FILE_SIZE),
                            .max_file_size =
                                get_config_value_ull(CFG_DIR_MAX_FILE_SIZE),
                            .max_storage_size =
                                get_config_value_ull(CFG_DIR_MAX_STORAGE_SIZE),
                            .max_chunk_size =
                                get_config_value_ull(CFG_DIR_MAX_CHUNK_SIZE),
                        },
                },
            .worker_thread_count =
                get_config_value_ull(CFG_DIR_WORKER_THREAD_COUNT),
        };
    }

    entrypoint_config get_entrypoint_config() {
        if (m_service_role != uh::cluster::ENTRYPOINT_SERVICE)
            throw std::invalid_argument(
                "Only service instances of the type '" +
                get_service_string(uh::cluster::ENTRYPOINT_SERVICE) +
                "' may access the " +
                get_service_string(uh::cluster::DEDUPLICATOR_SERVICE) +
                " ENTRYPOINT_SERVICE!");
        return {
            .dedupe_node_connection_count = get_config_value_ull(
                CFG_ENTRYPOINT_DEDUP_SERVICE_CONNECTION_COUNT),
            .directory_connection_count = get_config_value_ull(
                CFG_ENTRYPOINT_DIR_SERVICE_CONNECTION_COUNT),
            .worker_thread_count =
                get_config_value_ull(CFG_ENTRYPOINT_WORKER_THREAD_COUNT),
        };
    }

    [[nodiscard]] inline const std::string get_service_name() const noexcept {
        return m_service_name;
    }

private:
    const std::string m_identity_file = "identity";
    etcd::SyncClient& m_etcd_client;
    const uh::cluster::role m_service_role;
    const std::filesystem::path m_working_dir;
    const std::string m_service_name;

    class registry_lock {
    public:
        explicit registry_lock(etcd::SyncClient& client)
            : m_client(client) {
            m_response = m_client.lock(etcd_global_lock_key);
        }

        ~registry_lock() { m_client.unlock(m_response.lock_key()); }

    private:
        etcd::SyncClient& m_client;
        etcd::Response m_response;
    };

    void init_default_config_values() {
        // these are only default settings

        const auto lock =
            wait_for_success(ETCD_TIMEOUT, ETCD_RETRY_INTERVAL,
                             [this]() { return registry_lock(m_etcd_client); });

        if (!key_exists(etcd_initialized_key)) {
            set_class_config_value(uh::cluster::DEDUPLICATOR_SERVICE,
                                   uh::cluster::CFG_SERVER_PORT, 9300);
            set_class_config_value(uh::cluster::DEDUPLICATOR_SERVICE,
                                   uh::cluster::CFG_SERVER_BIND_ADDR,
                                   "0.0.0.0");
            set_class_config_value(uh::cluster::DEDUPLICATOR_SERVICE,
                                   uh::cluster::CFG_SERVER_THREADS, 4);
            set_class_config_value(uh::cluster::DEDUPLICATOR_SERVICE,
                                   uh::cluster::CFG_DEDUP_MIN_FRAGMENT_SIZE,
                                   32ul);
            set_class_config_value(uh::cluster::DEDUPLICATOR_SERVICE,
                                   uh::cluster::CFG_DEDUP_MAX_FRAGMENT_SIZE,
                                   8ul * 1024ul);
            set_class_config_value(uh::cluster::DEDUPLICATOR_SERVICE,
                                   uh::cluster::CFG_DEDUP_WORKER_MIN_DATA_SIZE,
                                   128ul * 1024ul);
            set_class_config_value(uh::cluster::DEDUPLICATOR_SERVICE,
                                   uh::cluster::CFG_DEDUP_WORKER_THREAD_COUNT,
                                   32ul);
            set_class_config_value(uh::cluster::DEDUPLICATOR_SERVICE,
                                   uh::cluster::CFG_GDV_READ_CACHE_CAPACITY_L1,
                                   8000000ul);
            set_class_config_value(uh::cluster::DEDUPLICATOR_SERVICE,
                                   uh::cluster::CFG_GDV_READ_CACHE_CAPACITY_L2,
                                   4000ul);
            set_class_config_value(uh::cluster::DEDUPLICATOR_SERVICE,
                                   uh::cluster::CFG_GDV_L1_SAMPLE_SIZE, 128ul);
            set_class_config_value(
                uh::cluster::DEDUPLICATOR_SERVICE,
                uh::cluster::CFG_GDV_STORAGE_SERVICE_CONNECTION_COUNT, 16ul);
            set_class_config_value(uh::cluster::DEDUPLICATOR_SERVICE,
                                   uh::cluster::CFG_GDV_MAX_DATA_STORE_SIZE,
                                   64ul * 1024ul * 1024ul * 1024ul);

            set_class_config_value(uh::cluster::DIRECTORY_SERVICE,
                                   uh::cluster::CFG_SERVER_PORT, 9400);
            set_class_config_value(uh::cluster::DIRECTORY_SERVICE,
                                   uh::cluster::CFG_SERVER_BIND_ADDR,
                                   "0.0.0.0");
            set_class_config_value(uh::cluster::DIRECTORY_SERVICE,
                                   uh::cluster::CFG_SERVER_THREADS, 4);
            set_class_config_value(uh::cluster::DIRECTORY_SERVICE,
                                   uh::cluster::CFG_DIR_MIN_FILE_SIZE,
                                   2ul * 1024ul * 1024ul * 1024ul);
            set_class_config_value(uh::cluster::DIRECTORY_SERVICE,
                                   uh::cluster::CFG_DIR_MAX_FILE_SIZE,
                                   64ul * 1024ul * 1024ul * 1024ul);
            set_class_config_value(uh::cluster::DIRECTORY_SERVICE,
                                   uh::cluster::CFG_DIR_MAX_STORAGE_SIZE,
                                   256ul * 1024ul * 1024ul * 1024ul);
            set_class_config_value(uh::cluster::DIRECTORY_SERVICE,
                                   uh::cluster::CFG_DIR_MAX_CHUNK_SIZE,
                                   std::numeric_limits<uint32_t>::max());
            set_class_config_value(uh::cluster::DIRECTORY_SERVICE,
                                   uh::cluster::CFG_DIR_WORKER_THREAD_COUNT,
                                   8ul);
            set_class_config_value(uh::cluster::DIRECTORY_SERVICE,
                                   uh::cluster::CFG_GDV_READ_CACHE_CAPACITY_L1,
                                   8000000ul);
            set_class_config_value(uh::cluster::DIRECTORY_SERVICE,
                                   uh::cluster::CFG_GDV_READ_CACHE_CAPACITY_L2,
                                   4000ul);
            set_class_config_value(uh::cluster::DIRECTORY_SERVICE,
                                   uh::cluster::CFG_GDV_L1_SAMPLE_SIZE, 128ul);
            set_class_config_value(
                uh::cluster::DIRECTORY_SERVICE,
                uh::cluster::CFG_GDV_STORAGE_SERVICE_CONNECTION_COUNT, 16ul);
            set_class_config_value(uh::cluster::DIRECTORY_SERVICE,
                                   uh::cluster::CFG_GDV_MAX_DATA_STORE_SIZE,
                                   64ul * 1024ul * 1024ul * 1024ul);

            set_class_config_value(uh::cluster::ENTRYPOINT_SERVICE,
                                   uh::cluster::CFG_SERVER_PORT, 8080);
            set_class_config_value(uh::cluster::ENTRYPOINT_SERVICE,
                                   uh::cluster::CFG_SERVER_BIND_ADDR,
                                   "0.0.0.0");
            set_class_config_value(uh::cluster::ENTRYPOINT_SERVICE,
                                   uh::cluster::CFG_SERVER_THREADS, 4);
            set_class_config_value(
                uh::cluster::ENTRYPOINT_SERVICE,
                uh::cluster::CFG_ENTRYPOINT_DEDUP_SERVICE_CONNECTION_COUNT,
                4ul);
            set_class_config_value(
                uh::cluster::ENTRYPOINT_SERVICE,
                uh::cluster::CFG_ENTRYPOINT_DIR_SERVICE_CONNECTION_COUNT, 4ul);
            set_class_config_value(
                uh::cluster::ENTRYPOINT_SERVICE,
                uh::cluster::CFG_ENTRYPOINT_WORKER_THREAD_COUNT, 12ul);

            set(etcd_initialized_key, "1");
        }
    }

    void set_class_config_value(const uh::cluster::role service_role,
                                const uh::cluster::config_parameter parameter,
                                std::size_t value) {
        set_class_config_value(service_role, parameter, std::to_string(value));
    }

    void set_class_config_value(const uh::cluster::role service_role,
                                const uh::cluster::config_parameter parameter,
                                const std::string& value) {
        std::string key = etcd_global_config_key_prefix +
                          get_service_string(service_role) + "/" +
                          get_config_string(parameter);
        set(key, value);
    }

    std::string
    get_class_config_value(const uh::cluster::config_parameter& parameter) {
        std::string global_key = etcd_global_config_key_prefix +
                                 get_service_string(m_service_role) + "/" +
                                 get_config_string(parameter);
        return get(global_key);
    }

    std::string
    get_config_value_string(const uh::cluster::config_parameter& parameter) {
        std::string key = etcd_instance_config_key_prefix + m_service_name +
                          "/" + get_config_string(parameter);
        try {
            return get(key);
        } catch (std::invalid_argument const& e_instance) {
            return get_class_config_value(parameter);
        }
    }

    std::size_t
    get_config_value_ull(const uh::cluster::config_parameter& parameter) {
        return std::stoull(get_config_value_string(parameter));
    }

    bool key_exists(const std::string& key) {
        try {
            get(key);
        } catch (std::invalid_argument const& e) {
            return false;
        }
        return true;
    }

    std::string get(const std::string& key) {
        const auto response =
            wait_for_success(ETCD_TIMEOUT, ETCD_RETRY_INTERVAL,
                             [this, &key]() { return m_etcd_client.get(key); });

        if (response.is_ok())
            return response.value().as_string();
        else
            throw std::invalid_argument(
                "retrieval of configuration parameter " + key +
                " failed, details: " + response.error_message());
    }

    std::string set(const std::string& key, const std::string& value) {
        const auto response = wait_for_success(
            ETCD_TIMEOUT, ETCD_RETRY_INTERVAL,
            [this, &key, &value]() { return m_etcd_client.set(key, value); });
        if (response.is_ok())
            return response.value().as_string();
        else
            throw std::invalid_argument(
                "setting the configuration parameter " + key +
                " failed, details: " + response.error_message());
    }
};

} // namespace uh::cluster

#endif // UH_CLUSTER_CONFIG_REGISTRY_H
