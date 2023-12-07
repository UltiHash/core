#include "entry_node.h"
#include "entry_node_rest_handler.h"

namespace uh::cluster
{

//------------------------------------------------------------------------------

entry_node::entry_node(int id, cluster_map cmap) :
        m_cluster_map (std::move (cmap)),
        m_id (id),
        m_job_name ("entry_" + std::to_string (id)),
        m_workers (std::make_shared <boost::asio::thread_pool> (m_cluster_map.m_cluster_conf.entry_node_conf.worker_thread_count)),
        m_rest_server (m_cluster_map.m_cluster_conf.entry_node_conf, m_dedupe_nodes, m_directory_nodes, m_workers)
{

    sleep(6);

    create_connections();
}

//------------------------------------------------------------------------------

void
entry_node::run()
{
    m_rest_server.run();
}

void entry_node::create_connections() {

    for (const auto& dedupe_node: m_cluster_map.m_roles.at(DEDUPE_NODE)) {
        m_dedupe_nodes.emplace_back (std::make_shared <client> (m_rest_server.get_executor(), dedupe_node.second,
                                     m_cluster_map.m_cluster_conf.dedupe_node_conf.server_conf.port,
                                     m_cluster_map.m_cluster_conf.entry_node_conf.dedupe_node_connection_count));
    }

    for (const auto& directory: m_cluster_map.m_roles.at(DIRECTORY_NODE)) {
        m_directory_nodes.emplace_back(std::make_shared <client> (m_rest_server.get_executor(), directory.second,
                                       m_cluster_map.m_cluster_conf.directory_node_conf.server_conf.port,
                                       m_cluster_map.m_cluster_conf.entry_node_conf.directory_connection_count));
    }
}

void entry_node::stop() {
    LOG_INFO() << "stopping " << m_job_name;
    m_workers->join();
    m_workers->stop();
}

//------------------------------------------------------------------------------

} // namespace uh::cluster