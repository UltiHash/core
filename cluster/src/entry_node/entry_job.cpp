#include "entry_job.h"

namespace uh::cluster
{

//------------------------------------------------------------------------------

entry_job::entry_job(int id, cluster_map&& cmap) :
        m_cluster_map (std::move (cmap)),
        m_io_service (m_cluster_map.m_cluster_conf.entry_node_conf.internal_server_conf.threads),
        m_id (id),
        m_job_name ("entry_" + std::to_string (id)),
        m_internal_server (m_cluster_map.m_cluster_conf.entry_node_conf.internal_server_conf),
        m_rest_server (m_cluster_map.m_cluster_conf.entry_node_conf.rest_server_conf)
{
    sleep(4);
    create_connections();
    m_io_service.run();
}

//------------------------------------------------------------------------------

void
entry_job::run()
{

    std::string msg ("hello cluster");

    //std::cout << std::string_view (resp.data.get(), resp.size) << std::endl;

    m_rest_server.run();
    m_internal_server.run();
}

void entry_job::create_connections() {

    for (const auto& dedupe_node: m_cluster_map.m_roles.at(DEDUPE_NODE)) {
        m_dedupe_nodes.emplace_back (m_io_service, dedupe_node.second,
                                     m_cluster_map.m_cluster_conf.dedupe_node_conf.server_conf.port,
                                     m_cluster_map.m_cluster_conf.entry_node_conf.dedupe_node_connection_count);
    }

    for (const auto& phonebook: m_cluster_map.m_roles.at(PHONE_BOOK_NODE)) {
        m_phonebooks.emplace_back(m_io_service, phonebook.second,
                                  m_cluster_map.m_cluster_conf.phonebook_node_conf.server_conf.port,
                                  m_cluster_map.m_cluster_conf.entry_node_conf.phonebook_connection_count);
    }
}

//------------------------------------------------------------------------------

} // namespace uh::cluster