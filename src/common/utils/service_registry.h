//
// Created by max on 13.12.23.
//

#ifndef UH_CLUSTER_SERVICE_REGISTRY_H
#define UH_CLUSTER_SERVICE_REGISTRY_H

#include <shared_mutex>
#include <string>
#include "third-party/etcd-cpp-apiv3/etcd/Client.hpp"
#include "third-party/etcd-cpp-apiv3/etcd/KeepAlive.hpp"
#include <boost/asio.hpp>

#include "common.h"
#include "log.h"
#include "etcd/Watcher.hpp"

namespace uh::cluster {

    enum class etcd_action : uint8_t {
        create = 0,
        erase,
    };

    static etcd_action get_etcd_action_enum(const std::string &action_str) {
        const std::map<std::string, etcd_action> etcd_action = {
                {"create", etcd_action::create},
                {"delete", etcd_action::erase},
        };

        if (etcd_action.contains(action_str))
            return etcd_action.at(action_str);
        else
            throw std::invalid_argument("invalid etcd action");
    }

    typedef struct {
        uh::cluster::role role;
        std::size_t id;
        std::string value;
    } service_info;

    static service_info extract_service_info(const std::string &key, const std::string& value) {
        size_t uh_slash_pos = key.find("/uh/");
        size_t next_slash_pos = key.find('/', uh_slash_pos + 4);

        return { .role = get_service_role(key.substr(uh_slash_pos + 4, next_slash_pos - uh_slash_pos - 4)),
                .id = std::stoul(key.substr(next_slash_pos+1)),
                .value = value};
    };

    class service_registry {
    public:
        service_registry(std::string  service_id, const std::string& etcd_host) :
            m_etcd_host(etcd_host),
            m_service_id(std::move(service_id)),
            m_etcd_client(m_etcd_host),
            m_watcher(m_etcd_host, m_etcd_default_key_prefix, [this](etcd::Response response) {return handle_state_changes(response);}, true)
        {
        }

        void handle_state_changes(const etcd::Response& response)
        {
            LOG_DEBUG() << "action: " << response.action() << ", key: " << response.value().key() << ", value: " << response.value().as_string();

            auto service_info = extract_service_info(response.value().key(), response.value().as_string());

            switch (get_etcd_action_enum(response.action())) {
                case etcd_action::create:
                    if (m_add_service_callbacks.contains(service_info.role))
                        m_add_service_callbacks[service_info.role](service_info);
                    break;

                case etcd_action::erase:
                    if (m_remove_service_callbacks.contains(service_info.role))
                        m_remove_service_callbacks[service_info.role](service_info);
                    break;
            }
        }

        void register_callback_add_service(uh::cluster::role service_role, std::function<void(const service_info&)> callback) {
            m_add_service_callbacks[service_role] = std::move(callback);
        }

        void register_callback_remove_service(uh::cluster::role service_role, std::function<void(const service_info&)> callback) {
            m_remove_service_callbacks[service_role] = std::move(callback);
        }

        class registration
        {
        public:
            registration(etcd::Client& client, const std::string& key, std::size_t ttl)
                : m_client(client),
                  m_lease(m_client.leasegrant(ttl).get().value().lease()),
                  m_keepalive(m_client, ttl, m_lease)
            {
                m_client.set(key, boost::asio::ip::host_name(), m_lease);
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
                m_etcd_default_ttl);
        }

        std::vector<std::pair<std::size_t, std::string>> get_service_instances(uh::cluster::role service_role) {
            std::vector<std::pair<std::size_t, std::string>> result;

            std::string service_key = m_etcd_default_key_prefix + get_service_string(service_role);
            etcd::Response service_instances = m_etcd_client.ls(service_key).get();
            for (size_t i = 0; i < service_instances.keys().size(); i++) {
                const auto& service_instance = service_instances.value(i);
                std::filesystem::path service_path(service_instance.key());
                try {
                    std::size_t service_index = std::stoull(service_path.filename().string());
                    result.emplace_back(service_index, service_instance.as_string());
                } catch (std::invalid_argument& e) {
                    LOG_ERROR() << "Failed to extract instance id from key " << service_key << ".";
                    return result;
                }
            }

            return result;
        }

        void wait_for_dependency(uh::cluster::role dependency) {
            std::string dependency_key = m_etcd_default_key_prefix + get_service_string(dependency);

            while(m_etcd_client.ls(dependency_key).get().keys().empty()) {
                LOG_INFO() << "Waiting for dependency " << dependency_key << " to become available...";
                sleep(5);
            }
            LOG_INFO() << "Dependency " << dependency_key << " seems to be available.";
        }

    private:
    #ifdef NDEBUG
        const std::size_t m_etcd_default_ttl = 5;
    #else
        // this value is used when running a debug build
        // having a longer ttl is important here as running into a breakpoint will almost
        // immediately let you run into ttl and thus will make debugging more complicated
        const std::size_t m_etcd_default_ttl = 60000;
    #endif

        const std::string m_etcd_default_key_prefix = "/uh/";

        const std::string m_etcd_host;
        const std::string m_service_id;

        etcd::Client m_etcd_client;

        etcd::Watcher m_watcher;
        std::map<uh::cluster::role, std::function<void(const service_info&)>> m_add_service_callbacks;
        std::map<uh::cluster::role, std::function<void(const service_info&)>> m_remove_service_callbacks;

    };

}

#endif //UH_CLUSTER_SERVICE_REGISTRY_H
