#ifndef CORE_DEDUPE_NODE_H
#define CORE_DEDUPE_NODE_H

#include <functional>
#include <iostream>
#include <utility>
#include "common/cluster_config.h"
#include "global_data/global_data_view.h"
#include "dedupe_node_handler.h"
#include <common/log.h>



namespace uh::cluster {
    class deduplication_service: service_interface {
    public:

        deduplication_service(std::size_t id, std::string  service_key, const std::string& etcd_host, const bool use_id_as_port_offset = false) :
                m_id(id),
                m_service_key(std::move(service_key)),
                m_service_name(m_service_key + "/" + std::to_string(m_id)),
                m_etcd_client(etcd_host),
                m_dedupe_workers (std::make_shared <boost::asio::thread_pool> (make_dedupe_node_config().worker_thread_count)),
                m_storage (m_etcd_client),
                m_server (make_dedupe_node_config().server_conf, m_service_name,
                          std::make_unique <dedupe_node_handler>(make_dedupe_node_config(), m_storage, m_dedupe_workers)),
                m_use_id_as_port_offset (use_id_as_port_offset)
        {}

        void run() override {
            //TODO: wait for dependencies to be available before creating connections
            m_storage.create_data_node_connections(m_server.get_executor(), m_use_id_as_port_offset);
            register_service();
            m_server.run();
        }

        void stop() override {
            m_server.stop();
            m_dedupe_workers->join();
            m_dedupe_workers->stop();
            unregister_service();
        }

        ~deduplication_service() override {
            LOG_DEBUG() << "terminating " << m_service_name;
        }

        global_data_view& get_global_data_view() {
            return m_storage;
        }

        server& get_server() {
            return m_server;
        }

    private:
        const std::size_t m_id;
        const std::string m_service_key;
        const std::string m_service_name;

        etcd::Client m_etcd_client;
        std::shared_ptr<etcd::KeepAlive> m_etcd_keepalive;

        std::shared_ptr <boost::asio::thread_pool> m_dedupe_workers;
        global_data_view m_storage;
        server m_server;
        const bool m_use_id_as_port_offset;

        void register_service() {
            m_etcd_keepalive = m_etcd_client.leasekeepalive(etcd_default_ttl).get();
            std::string key = etcd_default_key_prefix + m_service_name;
            m_etcd_client.set(key, boost::asio::ip::host_name(), m_etcd_keepalive->Lease());
        }

        void unregister_service() {
            m_etcd_keepalive->Cancel();
        }

    };
} // end namespace uh::cluster

#endif //CORE_DEDUPE_NODE_H
