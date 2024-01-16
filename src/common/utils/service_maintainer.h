#ifndef UH_CLUSTER_SERVICE_MAINTAINER_H
#define UH_CLUSTER_SERVICE_MAINTAINER_H

#include <ranges>
#include <utility>

#include "log.h"
#include "common/network/client.h"
#include "common/utils/service_registry.h"
#include "entrypoint/rest_server.h"

namespace uh::cluster {

    template <typename T>
    class services {
    public:

        services (role r, service_registry& registry, std::shared_ptr<boost::asio::io_context> ioc):
        m_registry(registry), m_ioc(std::move(ioc)), m_role(r) {
            m_registry.register_callback_add_service(m_role, [this](const service_info& service) { add_service_callback(service); });
            m_registry.register_callback_remove_service(m_role, [this](const service_info& service) { remove_service_callback(service); });
        }

        // first create the client
        // then communicate the attribute
        // then add it to the map
        // for now we don't do this but calculate the offset from the id in the global data view

        // remove these public
//        void add_node_client(const std::size_t id, const std::string& address) {
//            std::lock_guard<std::mutex> lk(m_mutex);
//            m_clients.insert({id, std::make_shared<client>(m_ioc, address,
//                                                           make_deduplicator_config().server_conf.port,
//                                                           make_entrypoint_config().dedupe_node_connection_count)});
//        }

        // remove this
        void remove_node_client(const std::size_t id) {
            std::lock_guard<std::mutex> lk(m_mutex);
            m_clients.erase(id);
        }

        // filter : general filter, filter by property (eg: offset, prefixes), templated function
        std::shared_ptr<client> filter(T property) {

        }
        // crpt design pattern

        // inline and const when doing upper bound and find, mark it noexcept

        // function that gives the iterator of the map "const" end this is const expr noexcept inline function

        // function that gives the returns "const" begin iterator of the map: this is const expr noexcept inline function

        // getting elements by the service id

        [[nodiscard]] std::vector <std::shared_ptr <client>> get_clients_list() const {

            std::vector <std::shared_ptr <client>> clients_list;
            clients_list.reserve(m_clients.size());
            std::ranges::copy(m_clients | std::views::values, std::back_inserter(clients_list));

            return clients_list;
        }

    private:
        std::mutex m_mutex;
        std::map <size_t, std::shared_ptr <client>> m_clients; // make the map templated for size_t and const T
//        std::map <const T, std::shared_ptr <client>> m_data_node_offsets;

        service_registry& m_registry;
        std::shared_ptr<boost::asio::io_context> m_ioc;
        role m_role;

        // maintain the offset to client map here.

        void add_service_callback(const service_info& service) {
            LOG_INFO() << "add callback for service " << get_service_string(m_role) << " called.";

//            add_node_client(service.id, service.value);
            // function that communicates with the client and asks for the property
        }

        void remove_service_callback(const service_info& service) {
            LOG_INFO() << "removed callback for service " << get_service_string(m_role) << " called.";
            remove_node_client(service.id);
        }
    };

} // uh::cluster

#endif // UH_CLUSTER_SERVICE_MAINTAINER_H