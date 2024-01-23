//
// Created by max on 17.01.24.
//

#ifndef UH_CLUSTER_CONFIG_REGISTRY_H
#define UH_CLUSTER_CONFIG_REGISTRY_H

#include <string>
#include "third-party/etcd-cpp-apiv3/etcd/Client.hpp"
#include "common/utils/common.h"
#include "common/utils/cluster_config.h"
#include "namespace.h"


namespace uh::cluster {

class config_registry {

public:
    config_registry(uh::cluster::role role, const std::string& etcd_host) :
            m_etcd_client(etcd_host),
            m_service_role(role),
            m_service_id(generate_service_id()),
            m_service_name(get_service_string(m_service_role) + "/" + std::to_string(m_service_id))
    {
        init_default_config_values();
    }

    [[nodiscard]] std::size_t get_service_id() const noexcept {
        return m_service_id;
    }

    server_config get_server_config() {
        return {
                .threads = std::stoull(get_config_value(CFG_SERVER_THREADS)),
                .port = static_cast<uint16_t>(std::stoull(get_config_value(CFG_SERVER_PORT))),
                .bind_address = get_config_value(CFG_SERVER_BIND_ADDR)
        };
    }

    global_data_view_config get_global_data_view_config() {
        return make_global_data_view_config();
    }

    storage_config get_storage_config() {
        return make_storage_config();
    }

private:
    etcd::Client m_etcd_client;
    const uh::cluster::role m_service_role;
    const std::size_t m_service_id;
    const std::string m_service_name;




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

    std::size_t generate_service_id() {
        std::string current_id_key = m_etcd_current_id_prefix_key + get_service_string(m_service_role);
        registry_lock lock(m_etcd_client);

        if(!key_exists(current_id_key)) {
            set(current_id_key, std::to_string(0));
            return 0;
        }

        std::size_t current_id = std::stoull(get(current_id_key));
        current_id++;
        set(current_id_key, std::to_string(current_id));
        return current_id;

    }

    void init_default_config_values() {
        //these are only default settings
        //TODO: check if config file is available and use values from there if available
        registry_lock lock(m_etcd_client);
        if(!key_exists(m_etcd_initialized_key)) {
            set_config_global_value(uh::cluster::STORAGE_SERVICE, uh::cluster::CFG_SERVER_PORT, 9200);
            set_config_global_value(uh::cluster::STORAGE_SERVICE, uh::cluster::CFG_SERVER_BIND_ADDR, "0.0.0.0");
            set_config_global_value(uh::cluster::STORAGE_SERVICE, uh::cluster::CFG_SERVER_THREADS, 16);

            set_config_global_value(uh::cluster::DEDUPLICATOR_SERVICE, uh::cluster::CFG_SERVER_PORT, 9300);
            set_config_global_value(uh::cluster::DEDUPLICATOR_SERVICE, uh::cluster::CFG_SERVER_BIND_ADDR, "0.0.0.0");
            set_config_global_value(uh::cluster::DEDUPLICATOR_SERVICE, uh::cluster::CFG_SERVER_THREADS, 4);

            set_config_global_value(uh::cluster::DIRECTORY_SERVICE, uh::cluster::CFG_SERVER_PORT, 9400);
            set_config_global_value(uh::cluster::DIRECTORY_SERVICE, uh::cluster::CFG_SERVER_BIND_ADDR, "0.0.0.0");
            set_config_global_value(uh::cluster::DIRECTORY_SERVICE, uh::cluster::CFG_SERVER_THREADS, 4);

            set_config_global_value(uh::cluster::ENTRYPOINT_SERVICE, uh::cluster::CFG_SERVER_PORT, 8080);
            set_config_global_value(uh::cluster::ENTRYPOINT_SERVICE, uh::cluster::CFG_SERVER_BIND_ADDR, "0.0.0.0");
            set_config_global_value(uh::cluster::ENTRYPOINT_SERVICE, uh::cluster::CFG_SERVER_THREADS, 4);

            m_etcd_client.set(m_etcd_initialized_key, "1");
        }
    }

    void set_config_global_value(const uh::cluster::role service_role, const uh::cluster::config_parameter parameter, std::size_t value) {
        set_config_global_value(service_role, parameter, std::to_string(value));
    }

    void set_config_global_value(const uh::cluster::role service_role, const uh::cluster::config_parameter parameter, const std::string& value) {
        std::string key = m_etcd_global_config_key_prefix +
                          get_service_string(service_role) + "/" +
                          get_config_string(parameter);
        set(key, value);
    }

    std::string get_config_value(const uh::cluster::config_parameter parameter) {
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
