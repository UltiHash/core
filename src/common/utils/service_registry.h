//
// Created by max on 13.12.23.
//

#ifndef UH_CLUSTER_SERVICE_REGISTRY_H
#define UH_CLUSTER_SERVICE_REGISTRY_H

#include <string>
#include "third-party/etcd-cpp-apiv3/etcd/Client.hpp"
#include "third-party/etcd-cpp-apiv3/etcd/KeepAlive.hpp"
#include <boost/asio.hpp>
#include <utility>

#include "common.h"
#include "cluster_config.h"
#include "log.h"


namespace uh::cluster {

    class service_registry {

    public:
        service_registry(uh::cluster::role role, std::size_t index, std::string etcd_host) :
                m_etcd_host(std::move(etcd_host)),
                m_service_role(role),
                m_service_index(index),
                m_service_id(get_service_string(role) + "/" + std::to_string(index)),
                m_etcd_client(m_etcd_host),
                m_etcd_keepalive(m_etcd_client.leasekeepalive(m_etcd_default_ttl).get())
        {
            init_default_config_values();
        }

        [[nodiscard]] const std::string& get_service_name() const {
            return m_service_id;
        }

        [[nodiscard]] std::size_t get_service_index() const {
            return m_service_index;
        }

        class registration
        {
        public:
            registration(etcd::Client& client, const std::string& key, const std::string& value, std::size_t ttl)
                : m_client(client),
                  m_lease(m_client.leasegrant(ttl).get().value().lease()),
                  m_keepalive(m_client, ttl, m_lease)
            {
                m_etcd_client.set(key, value, m_lease);
            }

            ~registration()
            {
                m_client.leaserevoke(m_lease);
            }

        private:
            etcd::Client& m_client;
            int64_t m_lease;
            etcd::KeepAlive m_keepalive;
        };

        std::unique_ptr<registration> register_service() {
            return std::make_unique<registration>(
                m_etcd_client,
                m_etcd_default_key_prefix + m_service_id,
                boost::asio::ip::host_name(),
                m_etcd_default_ttl);
        }
      
        

        std::unique_ptr<registration> register_testing() {
            return std::make_unique<registration>(
                m_etcd_client,
                m_etcd_default_key_prefix + "global/testing",
                to_string(true),
                m_etcd_default_ttl);
        }
      
        bool detect_testing() {
            std::string key = m_etcd_default_key_prefix + "global/testing";
            auto response = m_etcd_client.get(key).get();
            try {
                if (response.is_ok())
                    return true;
                else
                    return false;
            } catch (std::exception const & ex) {
                throw std::system_error(EIO, std::generic_category(), "getting the key '" + key + "' failed due to communication problem, details: " + ex.what());
            }
        }

        server_config get_server_config() {
            std::string address = get_config_value(CFG_SERVER_BIND_ADDR);

            std::string port_str = get_config_value(CFG_SERVER_PORT);
            std::uint16_t port = 0;
            if(is_valid_pos_integer(port_str)){
                port = std::stoull(port_str);
                if(detect_testing())
                    port += m_service_index;
            }

            std::string threads_str = get_config_value(CFG_SERVER_THREADS);
            std::size_t threads = 0;
            if(is_valid_pos_integer(threads_str))
                threads = std::stoull(threads_str);

            return {
                .threads = threads,
                .port = port,
                .bind_address = address
            };
        }



        ~service_registry() {
            if(m_etcd_keepalive != NULL) {
                std::string key;
                if(m_registered)
                    key =  m_etcd_default_key_prefix + m_service_id + "/";
                else if (m_testing)
                    key = m_etcd_default_key_prefix + "global/testing";
                m_etcd_client.rmdir(key, true);
                m_etcd_keepalive->Cancel();
                LOG_INFO() << "deregistered service instance " << key << " from service registry.";
            }
        }

        std::string get_config_value(const uh::cluster::config_parameter parameter) {
            std::string key = m_etcd_default_key_prefix + m_service_id + "/" + get_cfg_param_string(parameter);
            try {
                return get(key);
            } catch (std::invalid_argument const & e_instance) {
                std::string global_key = m_etcd_default_key_prefix +
                        "global/" +
                        get_service_string(m_service_role) + "/" +
                        get_cfg_param_string(parameter);
                return get(global_key);
            }

        std::vector<service_endpoint> get_service_instances(uh::cluster::role service_role) {
            std::map<std::size_t, service_endpoint> endpoints_by_id;

            std::filesystem::path service_key(m_etcd_default_key_prefix + get_service_string(service_role));
            etcd::Response service_instances = m_etcd_client.ls(service_key.string()).get();
            for (int i = 0; i < service_instances.keys().size(); i++) {
                const auto& service_instance = service_instances.value(i);
                std::filesystem::path service_full_path(service_instance.key());
                std::filesystem::path service_rel_path = std::filesystem::relative(service_full_path, service_key);
                std::string top_element = service_rel_path.begin()->string();
                if(is_valid_pos_integer(top_element)) {
                    std::size_t service_index = std::stoull(top_element);
                    if (!endpoints_by_id.contains(service_index))
                        endpoints_by_id.emplace(std::size_t(service_index),
                                                service_endpoint{.role = service_role, .id = service_index});
                    auto &endpoint = endpoints_by_id.at(service_index);
                    if (service_rel_path.filename().string() == get_cfg_param_string(uh::cluster::CFG_ENDPOINT_HOST)) {
                        endpoint.host = service_instance.as_string();
                    } else if (service_rel_path.filename().string() == get_cfg_param_string(uh::cluster::CFG_ENDPOINT_PORT)) {
                        endpoint.port = std::stoul(service_instance.as_string());
                    }
                }
            }

            std::vector<service_endpoint> result;
            result.reserve(endpoints_by_id.size());

            std::transform(endpoints_by_id.begin(), endpoints_by_id.end(), std::back_inserter(result),
                           [](const auto& pair) { return pair.second; });

            return result;
        }

        void wait_for_dependency(uh::cluster::role dependency) {
            std::filesystem::path dependency_key(m_etcd_default_key_prefix + get_service_string(dependency));

            bool instance_found = false;
            while(!instance_found) {
                LOG_INFO() << "waiting for dependency " << dependency_key.string() << " to become available...";
                etcd::Response dependency_instances = m_etcd_client.ls(dependency_key.string()).get();
                for (int i = 0; i < dependency_instances.keys().size(); i++) {
                    const auto &dependency_instance = dependency_instances.value(i);
                    std::filesystem::path dependency_full_path(dependency_instance.key());
                    std::filesystem::path dependency_rel_path = std::filesystem::relative(dependency_full_path, dependency_key);
                    if(is_valid_pos_integer(dependency_rel_path.begin()->string())) {
                        instance_found = true;
                        break;
                    }
                }
                sleep(5);
            }
            LOG_INFO() << "dependency " << dependency_key.string() << " seems to be available.";
        }

    private:
        bool key_exists(std::string key) {
            try {
                get(key);
            } catch (std::invalid_argument const & e)  {
                return false;
            }
            return true;
        }

        void init_default_config_values() {
            auto response = m_etcd_client.lock(m_etcd_default_key_prefix + "global/lock").get();
            if(!key_exists(m_etcd_default_key_prefix + "global/initialized")) {
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

                m_etcd_client.set(m_etcd_default_key_prefix + "global/initialized", "1");

            }
            auto _ = m_etcd_client.unlock(response.lock_key()).get();
        }

        void set_config_global_value(const uh::cluster::role service_role, const uh::cluster::config_parameter parameter, std::size_t value) {
            set_config_global_value(service_role, parameter, std::to_string(value));
        }

        void set_config_global_value(const uh::cluster::role service_role, const uh::cluster::config_parameter parameter, const std::string& value) {
            std::string key = m_etcd_default_key_prefix +
                              "global/" +
                              get_service_string(service_role) + "/" +
                              get_cfg_param_string(parameter);
            set(key, value);
        }

        static bool is_valid_pos_integer(const std::string& str) {
            // Check if the string is empty
            if (str.empty()) {
                return false;
            }

            // Check if each character is a digit
            for (char c : str) {
                if (!std::isdigit(c)) {
                    return false;
                }
            }

            // Use std::stoi to check if the integer value is in the valid range
            try {
                std::size_t pos;
                std::size_t value = std::stoull(str, &pos);

                // Ensure the entire string was converted
                if (pos != str.length())
                    return false;

                return true;
            } catch (const std::out_of_range&) {
                // std::stoull threw an out_of_range exception
                return false;
            } catch (const std::invalid_argument&) {
                // std::stoull threw an invalid_argument exception
                return false;
            }
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

        static std::string retrieve_hostname() {
            return boost::asio::ip::host_name();
        }

        const std::size_t m_etcd_default_ttl = 600;

        const std::string m_etcd_default_key_prefix = "/uh/";

        const std::string m_etcd_host;
        const uh::cluster::role m_service_role;
        const std::size_t m_service_index;
        const std::string m_service_id;

        etcd::Client m_etcd_client;
    };

}

#endif //UH_CLUSTER_SERVICE_REGISTRY_H
