//
// Created by max on 13.12.23.
//

#ifndef UH_CLUSTER_SERVICE_REGISTRY_H
#define UH_CLUSTER_SERVICE_REGISTRY_H

#include <string>
#include "third-party/etcd-cpp-apiv3/etcd/Client.hpp"
#include "third-party/etcd-cpp-apiv3/etcd/KeepAlive.hpp"
#include <boost/asio.hpp>

#include "common.h"
#include "log.h"
#include "etcd/Watcher.hpp"

namespace uh::cluster {

    namespace utils{
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
    } service_info;

    static service_info extract_service_info(const std::string &key) {
        size_t uh_slash_pos = key.find("/uh/");
        size_t next_slash_pos = key.find('/', uh_slash_pos + 4);

        return { .role = get_service_role(key.substr(uh_slash_pos + 4, next_slash_pos - uh_slash_pos - 4)),
                 .id = std::stoul(key.substr(next_slash_pos+1))};
    };
}

    class service_registry {

    public:
        service_registry(std::string service_id, const std::string& etcd_host) :
            m_etcd_host(etcd_host),
            m_service_id(std::move(service_id)),
            m_etcd_client(m_etcd_host),
            m_watcher(m_etcd_host, m_etcd_default_key_prefix, [this](etcd::Response response) {return handle_state_changes(response);}, true)
        {
        }

        void handle_state_changes(const etcd::Response& response)
        {
            LOG_DEBUG() << "action: " << response.action() << ", key: " << response.value().key() << ", value: " << response.value().as_string();

            auto service_info = utils::extract_service_info(response.value().key());
            const auto& service_role = service_info.role;
            const auto& service_id = service_info.id;

            switch (utils::get_etcd_action_enum(response.action())) {
                case utils::etcd_action::create:
                    if (m_add_service_callbacks.contains(service_role))
                        m_add_service_callbacks[service_role](service_id);
                    break;
                case utils::etcd_action::erase:
                    if (m_remove_service_callbacks.contains(service_role))
                        m_remove_service_callbacks[service_role](service_id);
                    break;
            }
        }

        void register_service() {
            m_etcd_keepalive = m_etcd_client.leasekeepalive(m_etcd_default_ttl).get();
            std::string key = m_etcd_default_key_prefix + m_service_id;
            m_etcd_client.set(key, boost::asio::ip::host_name(), m_etcd_keepalive->Lease());
        }

        void add_dedup()
        {
            m_etcd_client.set("/uh/deduplicator/1", "42");
        }

        void remove_dedup()
        {
            m_etcd_client.rmdir("/uh/deduplicator/1");
        }

        void register_callback_add_service(uh::cluster::role service_role, std::function<void(const std::size_t)> callback) {
            m_add_service_callbacks[service_role] = std::move(callback);
        }

        void register_callback_remove_service(uh::cluster::role service_role, std::function<void(const std::size_t)> callback) {
            m_remove_service_callbacks[service_role] = std::move(callback);
        }

        ~service_registry() {
            m_watcher.Cancel();
            if(m_etcd_keepalive != NULL)
                m_etcd_keepalive->Cancel();
        }

        std::vector<std::pair<std::size_t, std::string>> get_service_instances(uh::cluster::role service_role) {
            std::vector<std::pair<std::size_t, std::string>> result;

            std::string service_key = m_etcd_default_key_prefix + get_service_string(service_role);
            etcd::Response service_instances = m_etcd_client.ls(service_key).get();
            for (int i = 0; i < service_instances.keys().size(); i++) {
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

        std::map<uh::cluster::role, std::function<void(const std::size_t)>> m_add_service_callbacks;
        std::map<uh::cluster::role, std::function<void(const std::size_t)>> m_remove_service_callbacks;

        etcd::Client m_etcd_client;
        std::shared_ptr<etcd::KeepAlive> m_etcd_keepalive;

        etcd::Watcher m_watcher;
    };

}

#endif //UH_CLUSTER_SERVICE_REGISTRY_H
