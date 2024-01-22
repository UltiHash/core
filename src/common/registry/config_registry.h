//
// Created by max on 17.01.24.
//

#ifndef UH_CLUSTER_CONFIG_REGISTRY_H
#define UH_CLUSTER_CONFIG_REGISTRY_H

#include <string>
#include "third-party/etcd-cpp-apiv3/etcd/Client.hpp"
#include "common/utils/common.h"
#include "common/utils/cluster_config.h"
#include "common/utils/log.h"
#include "namespace.h"


namespace uh::cluster {

class config_registry {

public:
    config_registry(uh::cluster::role role, std::size_t index, std::string etcd_host) :
            m_service_name(get_service_string(role) + "/" + std::to_string(index)),
            m_etcd_host(std::move(etcd_host)),
            m_service_role(role),
            m_etcd_client(m_etcd_host)
    {
        init_default_config_values();
    }

    server_config get_server_config() {
        return {
                .threads = get_config_value_ull(CFG_SERVER_THREADS),
                .port = static_cast<uint16_t>(get_config_value_ull(CFG_SERVER_PORT)),
                .bind_address = get_config_value_string(CFG_SERVER_BIND_ADDR)
        };
    }

    global_data_view_config get_global_data_view_config() {
        if(m_service_role != uh::cluster::DEDUPLICATOR_SERVICE and m_service_role != uh::cluster::DIRECTORY_SERVICE)
            throw std::invalid_argument("Only service instances of the type '" + get_service_string(uh::cluster::DEDUPLICATOR_SERVICE) + "' or '" + get_service_string(uh::cluster::DIRECTORY_SERVICE) + "' may access the global data view configuration!");
        return {
            .storage_service_connection_count = get_config_value_ull(CFG_GDV_STORAGE_SERVICE_CONNECTION_COUNT),
            .read_cache_capacity_l1 = get_config_value_ull(CFG_GDV_READ_CACHE_CAPACITY_L1),
            .read_cache_capacity_l2 = get_config_value_ull(CFG_GDV_READ_CACHE_CAPACITY_L2),
            .l1_sample_size = get_config_value_ull(CFG_GDV_L1_SAMPLE_SIZE),
            .max_data_store_size = get_config_value_ull(CFG_GDV_MAX_DATA_STORE_SIZE),
        };
    }

    storage_config get_storage_config() {
        if(m_service_role != uh::cluster::STORAGE_SERVICE)
            throw std::invalid_argument("Only service instances of the type '" + get_service_string(uh::cluster::STORAGE_SERVICE) + "' may access the " + get_service_string(uh::cluster::STORAGE_SERVICE) + " configuration!");
        return {
            .root_dir = get_config_value_string(CFG_STORAGE_ROOT_DIR),
            .min_file_size = get_config_value_ull(CFG_STORAGE_MIN_FILE_SIZE),
            .max_file_size = get_config_value_ull(CFG_STORAGE_MAX_FILE_SIZE),
            .max_data_store_size = get_config_value_ull(CFG_STORAGE_MAX_DATA_STORE_SIZE),
        };
    }

    deduplicator_config get_deduplicator_config() {
        if(m_service_role != uh::cluster::DEDUPLICATOR_SERVICE)
            throw std::invalid_argument("Only service instances of the type '" + get_service_string(uh::cluster::DEDUPLICATOR_SERVICE) + "' may access the " + get_service_string(uh::cluster::DEDUPLICATOR_SERVICE) + " configuration!");
        return {
            .root_dir = get_config_value_string(CFG_DEDUP_ROOT_DIR),
            .min_fragment_size = get_config_value_ull(CFG_DEDUP_MIN_FRAGMENT_SIZE),
            .max_fragment_size = get_config_value_ull(CFG_DEDUP_MAX_FRAGMENT_SIZE),
            .dedupe_worker_minimum_data_size = get_config_value_ull(CFG_DEDUP_WORKER_MIN_DATA_SIZE),
            .worker_thread_count = get_config_value_ull(CFG_DEDUP_WORKER_THREAD_COUNT),
        };
    }

    directory_config get_directory_config() {
        if(m_service_role != uh::cluster::DIRECTORY_SERVICE)
            throw std::invalid_argument("Only service instances of the type '" + get_service_string(uh::cluster::DIRECTORY_SERVICE) + "' may access the " + get_service_string(uh::cluster::DEDUPLICATOR_SERVICE) + " DIRECTORY_SERVICE!");
        return {
                .directory_store_conf = {
                        .root_dir = get_config_value_string(CFG_DIR_ROOT_DIR),
                        .bucket_conf = {
                                .min_file_size = get_config_value_ull(CFG_DIR_MIN_FILE_SIZE),
                                .max_file_size = get_config_value_ull(CFG_DIR_MAX_FILE_SIZE),
                                .max_storage_size = get_config_value_ull(CFG_DIR_MAX_STORAGE_SIZE),
                                .max_chunk_size = get_config_value_ull(CFG_DIR_MAX_CHUNK_SIZE),
                        },
                },
                .worker_thread_count = get_config_value_ull(CFG_DIR_WORKER_THREAD_COUNT),
        };
    }

    entrypoint_config get_entrypoint_config() {
        if(m_service_role != uh::cluster::ENTRYPOINT_SERVICE)
            throw std::invalid_argument("Only service instances of the type '" + get_service_string(uh::cluster::ENTRYPOINT_SERVICE) + "' may access the " + get_service_string(uh::cluster::DEDUPLICATOR_SERVICE) + " ENTRYPOINT_SERVICE!");
        return {
                .dedupe_node_connection_count = get_config_value_ull(CFG_ENTRYPOINT_DEDUP_SERVICE_CONNECTION_COUNT),
                .directory_connection_count = get_config_value_ull(CFG_ENTRYPOINT_DIR_SERVICE_CONNECTION_COUNT),
                .worker_thread_count = get_config_value_ull(CFG_ENTRYPOINT_WORKER_THREAD_COUNT),
        };
    }

private:
    const std::string m_service_name;
    const std::string m_etcd_host;
    const uh::cluster::role m_service_role;
    etcd::Client m_etcd_client;



    class registry_lock
    {
    public:
        explicit registry_lock(etcd::Client& client) :
                m_client(client)
        {
            m_response = m_client.lock(m_etcd_global_lock_key).get();
        }

        ~registry_lock()
        {
            m_client.unlock(m_response.lock_key()).get();
        }

    private:
        etcd::Client& m_client;
        etcd::Response m_response;
    };

    void init_default_config_values() {
        //these are only default settings
        //TODO: check if config file is available and use values from there if available
        registry_lock lock(m_etcd_client);
        if(!key_exists(m_etcd_initialized_key)) {
            set_config_class_value(uh::cluster::STORAGE_SERVICE, uh::cluster::CFG_SERVER_PORT, 9200);
            set_config_class_value(uh::cluster::STORAGE_SERVICE, uh::cluster::CFG_SERVER_BIND_ADDR, "0.0.0.0");
            set_config_class_value(uh::cluster::STORAGE_SERVICE, uh::cluster::CFG_SERVER_THREADS, 16);

            set_config_class_value(uh::cluster::DEDUPLICATOR_SERVICE, uh::cluster::CFG_SERVER_PORT, 9300);
            set_config_class_value(uh::cluster::DEDUPLICATOR_SERVICE, uh::cluster::CFG_SERVER_BIND_ADDR, "0.0.0.0");
            set_config_class_value(uh::cluster::DEDUPLICATOR_SERVICE, uh::cluster::CFG_SERVER_THREADS, 4);

            set_config_class_value(uh::cluster::DIRECTORY_SERVICE, uh::cluster::CFG_SERVER_PORT, 9400);
            set_config_class_value(uh::cluster::DIRECTORY_SERVICE, uh::cluster::CFG_SERVER_BIND_ADDR, "0.0.0.0");
            set_config_class_value(uh::cluster::DIRECTORY_SERVICE, uh::cluster::CFG_SERVER_THREADS, 4);

            set_config_class_value(uh::cluster::ENTRYPOINT_SERVICE, uh::cluster::CFG_SERVER_PORT, 8080);
            set_config_class_value(uh::cluster::ENTRYPOINT_SERVICE, uh::cluster::CFG_SERVER_BIND_ADDR, "0.0.0.0");
            set_config_class_value(uh::cluster::ENTRYPOINT_SERVICE, uh::cluster::CFG_SERVER_THREADS, 4);

            set_config_class_value(uh::cluster::STORAGE_SERVICE, uh::cluster::CFG_STORAGE_ROOT_DIR, "/tmp/lib/uh/" +
                    get_service_string(uh::cluster::STORAGE_SERVICE));
            set_config_class_value(uh::cluster::STORAGE_SERVICE, uh::cluster::CFG_STORAGE_MIN_FILE_SIZE, 1ul * 1024ul * 1024ul * 1024ul);
            set_config_class_value(uh::cluster::STORAGE_SERVICE, uh::cluster::CFG_STORAGE_MAX_FILE_SIZE, 4ul * 1024ul * 1024ul * 1024ul);
            set_config_class_value(uh::cluster::STORAGE_SERVICE, uh::cluster::CFG_STORAGE_MAX_DATA_STORE_SIZE, 64ul * 1024ul * 1024ul * 1024ul);

            set_config_class_value(uh::cluster::DEDUPLICATOR_SERVICE, uh::cluster::CFG_DEDUP_ROOT_DIR, "/tmp/lib/uh/" +
                    get_service_string(uh::cluster::DEDUPLICATOR_SERVICE));
            set_config_class_value(uh::cluster::DEDUPLICATOR_SERVICE, uh::cluster::CFG_DEDUP_MIN_FRAGMENT_SIZE, 32ul);
            set_config_class_value(uh::cluster::DEDUPLICATOR_SERVICE, uh::cluster::CFG_DEDUP_MAX_FRAGMENT_SIZE, 8ul * 1024ul);
            set_config_class_value(uh::cluster::DEDUPLICATOR_SERVICE, uh::cluster::CFG_DEDUP_WORKER_MIN_DATA_SIZE, 128ul * 1024ul);
            set_config_class_value(uh::cluster::DEDUPLICATOR_SERVICE, uh::cluster::CFG_DEDUP_WORKER_THREAD_COUNT, 32ul);
            set_config_class_value(uh::cluster::DEDUPLICATOR_SERVICE, uh::cluster::CFG_GDV_READ_CACHE_CAPACITY_L1, 8000000ul);
            set_config_class_value(uh::cluster::DEDUPLICATOR_SERVICE, uh::cluster::CFG_GDV_READ_CACHE_CAPACITY_L2, 4000);
            set_config_class_value(uh::cluster::DEDUPLICATOR_SERVICE, uh::cluster::CFG_GDV_L1_SAMPLE_SIZE, 128ul);
            set_config_class_value(uh::cluster::DEDUPLICATOR_SERVICE, uh::cluster::CFG_GDV_STORAGE_SERVICE_CONNECTION_COUNT, 16ul);
            set_config_class_value(uh::cluster::DEDUPLICATOR_SERVICE, uh::cluster::CFG_GDV_MAX_DATA_STORE_SIZE, 64ul * 1024ul * 1024ul * 1024ul);


            set_config_class_value(uh::cluster::DIRECTORY_SERVICE, uh::cluster::CFG_DIR_ROOT_DIR, "/tmp/lib/uh/" +
                    get_service_string(uh::cluster::DIRECTORY_SERVICE));
            set_config_class_value(uh::cluster::DIRECTORY_SERVICE, uh::cluster::CFG_DIR_MIN_FILE_SIZE, 2ul * 1024ul * 1024ul * 1024ul);
            set_config_class_value(uh::cluster::DIRECTORY_SERVICE, uh::cluster::CFG_DIR_MAX_FILE_SIZE, 64ul * 1024ul * 1024ul * 1024ul);
            set_config_class_value(uh::cluster::DIRECTORY_SERVICE, uh::cluster::CFG_DIR_MAX_STORAGE_SIZE, 256ul * 1024ul * 1024ul * 1024ul);
            set_config_class_value(uh::cluster::DIRECTORY_SERVICE, uh::cluster::CFG_DIR_MAX_CHUNK_SIZE, std::numeric_limits <uint32_t>::max());
            set_config_class_value(uh::cluster::DIRECTORY_SERVICE, uh::cluster::CFG_DIR_WORKER_THREAD_COUNT, 8ul);
            set_config_class_value(uh::cluster::DIRECTORY_SERVICE, uh::cluster::CFG_GDV_READ_CACHE_CAPACITY_L1, 8000000ul);
            set_config_class_value(uh::cluster::DIRECTORY_SERVICE, uh::cluster::CFG_GDV_READ_CACHE_CAPACITY_L2, 4000);
            set_config_class_value(uh::cluster::DIRECTORY_SERVICE, uh::cluster::CFG_GDV_L1_SAMPLE_SIZE, 128ul);
            set_config_class_value(uh::cluster::DIRECTORY_SERVICE, uh::cluster::CFG_GDV_STORAGE_SERVICE_CONNECTION_COUNT, 16ul);
            set_config_class_value(uh::cluster::DIRECTORY_SERVICE, uh::cluster::CFG_GDV_MAX_DATA_STORE_SIZE, 64ul * 1024ul * 1024ul * 1024ul);

            set_config_class_value(uh::cluster::ENTRYPOINT_SERVICE, uh::cluster::CFG_ENTRYPOINT_DEDUP_SERVICE_CONNECTION_COUNT, 4ul);
            set_config_class_value(uh::cluster::ENTRYPOINT_SERVICE, uh::cluster::CFG_ENTRYPOINT_DIR_SERVICE_CONNECTION_COUNT, 4ul);
            set_config_class_value(uh::cluster::ENTRYPOINT_SERVICE, uh::cluster::CFG_ENTRYPOINT_WORKER_THREAD_COUNT, 12ul);

            m_etcd_client.set(m_etcd_initialized_key, "1");
        }
    }

    void set_config_class_value(const uh::cluster::role service_role, const uh::cluster::config_parameter parameter, std::size_t value) {
        set_config_class_value(service_role, parameter, std::to_string(value));
    }

    void set_config_class_value(const uh::cluster::role service_role, const uh::cluster::config_parameter parameter, const std::string& value) {
        std::string key = m_etcd_global_config_key_prefix +
                          get_service_string(service_role) + "/" +
                          get_config_string(parameter);
        set(key, value);
    }

    std::string get_config_value_string(const uh::cluster::config_parameter& parameter) {
        std::string key = m_etcd_instance_config_key_prefix +
                m_service_name + "/" +
                get_config_string(parameter);
        try {
            return get(key);
        } catch (std::invalid_argument const &e_instance) {
            std::string global_key = m_etcd_global_config_key_prefix +
                                     get_service_string(m_service_role) + "/" +
                                     get_config_string(parameter);
            return get(global_key);
        }
    }

    std::size_t get_config_value_ull(const uh::cluster::config_parameter& parameter) {
        return std::stoull(get_config_value_string(parameter));
    }

    bool key_exists(std::string key) {
        try {
            get(key);
        } catch (std::invalid_argument const & e)  {
            return false;
        }
        return true;
    }

    std::string get(const std::string &key) {
        etcd::Response response;

        try {
            response= m_etcd_client.get(key).get();
        } catch (std::exception const & ex) {
            throw std::system_error(EIO, std::generic_category(), "retrieval of configuration parameter " + key + " failed due to communication problem, details: " + ex.what());
        }

        if (response.is_ok())
            return response.value().as_string();
        else
            throw std::invalid_argument("retrieval of configuration parameter " + key + " failed, details: " + response.error_message());
    }

    std::string set(const std::string &key, const std::string &value) {
        auto response = m_etcd_client.set(key, value).get();
        try
        {
            if (response.is_ok())
                return response.value().as_string();
            else
                throw std::invalid_argument("setting the configuration parameter " + key + " failed, details: " + response.error_message());
        }
        catch (std::exception const & ex)
        {
            throw std::system_error(EIO, std::generic_category(), "setting the configuration parameter " + key + " failed due to communication problem, details: " + ex.what());
        }
    }
};

}

#endif //UH_CLUSTER_CONFIG_REGISTRY_H
