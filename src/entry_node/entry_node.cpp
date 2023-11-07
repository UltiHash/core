#include "entry_node.h"
#include "entry_node_internal_handler.h"
#include "entry_node_rest_handler.h"

namespace uh::cluster
{

//------------------------------------------------------------------------------

entry_node::entry_node(int id, cluster_map cmap) :
        m_cluster_map (std::move (cmap)),
        m_id (id),
        m_job_name ("entry_" + std::to_string (id)),
        m_internal_server (m_cluster_map.m_cluster_conf.entry_node_conf.internal_server_conf, m_job_name,
                           std::make_unique <entry_node_internal_handler>()),
        m_rest_server (m_cluster_map.m_cluster_conf.entry_node_conf.rest_server_conf, m_dedupe_nodes, m_directory_nodes)
{
    sleep(4);
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
        m_dedupe_nodes.emplace_back (m_rest_server.get_executor(), dedupe_node.second,
                                     m_cluster_map.m_cluster_conf.dedupe_node_conf.server_conf.port,
                                     m_cluster_map.m_cluster_conf.entry_node_conf.dedupe_node_connection_count);
    }

    for (const auto& directory: m_cluster_map.m_roles.at(DIRECTORY_NODE)) {
        m_directory_nodes.emplace_back(m_rest_server.get_executor(), directory.second,
                                       m_cluster_map.m_cluster_conf.directory_node_conf.server_conf.port,
                                       m_cluster_map.m_cluster_conf.entry_node_conf.directory_connection_count);
    }
}

void entry_node::stop() {
    throw std::runtime_error ("not implemented");
}

//------------------------------------------------------------------------------

} // namespace uh::cluster