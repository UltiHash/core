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
#include "log.h"

namespace uh::cluster {

    class service_registry {

    public:
        service_registry(uh::cluster::role role, std::size_t index, std::string etcd_host) :
                m_etcd_host(std::move(etcd_host)),
                m_service_role(role),
                m_service_index(index),
                m_service_id(get_service_string(role) + "/" + std::to_string(index)),
                m_etcd_client(m_etcd_host)
        {
        }

        [[nodiscard]] const std::string& get_service_id() const {
            return m_service_id;
        }

        void register_service(std::uint16_t port = 0) {
            m_etcd_keepalive = m_etcd_client.leasekeepalive(m_etcd_default_ttl).get();
            std::string key_base = m_etcd_default_key_prefix + m_service_id + "/";
            std::string key_host = key_base + get_cfg_param_string(uh::cluster::CFG_HOST);
            std::string key_port = key_base + get_cfg_param_string(uh::cluster::CFG_PORT);
            m_etcd_client.set(key_host, boost::asio::ip::host_name(), m_etcd_keepalive->Lease());
            m_etcd_client.set(key_port, std::to_string(port), m_etcd_keepalive->Lease());
            m_registered = true;
            LOG_INFO() << "registered service instance " << key_base << " in service registry.";
        }

        ~service_registry() {
            if(m_registered && m_etcd_keepalive != NULL) {
                std::string key_base = m_etcd_default_key_prefix + m_service_id + "/";
                m_etcd_client.rmdir(key_base, true);
                m_etcd_keepalive->Cancel();
                LOG_INFO() << "deregistered service instance " << m_etcd_default_key_prefix + m_service_id << " from service registry.";
            }
        }

        template <typename T, typename = std::enable_if_t<(std::is_integral_v<T> && std::is_unsigned_v<T>) || std::is_same_v<T, std::string>>>
        T get_config_value(const uh::cluster::config_parameter parameter) {
            std::string instance_key = m_etcd_default_key_prefix + m_service_id + "/" + get_cfg_param_string(parameter);
            try {
                return convert_to_type(get(instance_key));
            } catch (std::invalid_argument const & e_instance) {
                try {
                    std::string global_key = m_etcd_default_key_prefix + get_service_string(m_service_role) + "/global/" +
                            get_cfg_param_string(parameter);
                    return convert_to_type(get(global_key));
                } catch (std::invalid_argument const & e_global) {
                    //TODO: handle case that no values are in registry
                    return convert_to_type("0");
                }
            }
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
                    if (service_rel_path.filename().string() == get_cfg_param_string(uh::cluster::CFG_HOST)) {
                        endpoint.host = service_instance.as_string();
                    } else if (service_rel_path.filename().string() == get_cfg_param_string(uh::cluster::CFG_PORT)) {
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
            auto response = m_etcd_client.get(key).get();
            try
            {
                if (response.is_ok())
                    return response.value().as_string();
                else
                    throw std::invalid_argument("retrieval of configuration parameter " + key + " failed, details: " + response.error_message());
            }
            catch (std::exception const & ex)
            {
                throw std::system_error(EIO, std::generic_category(), "retrieval of configuration parameter " + key + " failed due to communication problem, details: " + ex.what());
            }

        }

        template <typename T, typename = std::enable_if_t<std::is_integral_v<T> && std::is_unsigned_v<T>>>
        T convert_to_type(const std::string& str) {
            if(is_valid_pos_integer(str))
                return static_cast<T>(std::stoull(str));
            else
                throw std::invalid_argument("the configuration value " + str + " does not appear to be a valid non-negative integer.");
        }

        std::string convert_to_type(const std::string& str) {
            return str;
        }

    #ifdef NDEBUG
        const std::size_t m_etcd_default_ttl = 5;
    #else
        // this value is used when running a debug build
        // having a longer ttl is important here as running into a breakpoint will almost
        // immediately let you run into ttl and thus will make debugging more complicated
        const std::size_t m_etcd_default_ttl = 60;
    #endif

        const std::string m_etcd_default_key_prefix = "/uh/";

        const std::string m_etcd_host;
        const uh::cluster::role m_service_role;
        const std::size_t m_service_index;
        const std::string m_service_id;

        etcd::Client m_etcd_client;
        std::shared_ptr<etcd::KeepAlive> m_etcd_keepalive;
        bool m_registered = false;

    };

}

#endif //UH_CLUSTER_SERVICE_REGISTRY_H
