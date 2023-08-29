////
//// Created by masi on 7/17/23.
////
//
//#ifndef CORE_DEDUPE_JOB_H
//#define CORE_DEDUPE_JOB_H
//
//#include <functional>
//#include <iostream>
//#include <utility>
//#include "cluster_config.h"
//#include "global_data.h"
//#include "paged_redblack_tree.h"
//
//namespace uh::cluster {
//class dedupe_job {
//public:
//
//    dedupe_job (int id, dedupe_config conf, cluster_ranks cluster_plan):
//            m_cluster_plan (std::move (cluster_plan)),
//            m_id (id),
//            m_job_name ("dedupe_node_" + std::to_string (id)),
//            m_dedupe_conf(std::move(conf)),
//            m_storage (m_dedupe_conf.storage_conf, m_cluster_plan.data_node_ranks),
//            m_fragment_set (m_dedupe_conf.set_conf, m_storage) {
//
//    }
//
//    void run() {
//        std::cout << "hello from " << m_job_name << std::endl;
//
//        MPI_Status status;
//        int message_size;
//
//        while (!m_stop) {
//
//            MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
//
//            try {
//                switch (status.MPI_TAG) {
//                    case message_types::DEDUPE_REQ:
//
//                        MPI_Get_count (&status, MPI_CHAR, &message_size);
//                        handle_dedupe (status.MPI_SOURCE, message_size);
//                        break;
//                    default:
//                        throw std::invalid_argument("Unknown tag");
//                }
//            }
//            catch (std::exception& e) {
//                handle_failure (m_job_name, status.MPI_SOURCE, e);
//            }
//        }
//    }
//
//    void handle_dedupe (int source, int data_size) {
//        MPI_Status status;
//        const auto buf = std::make_unique_for_overwrite<char []> (data_size);
//        std::cout << "before recv" << std::endl;
//
//        auto rc = MPI_Recv(buf.get(), data_size, MPI_CHAR, source, message_types::DEDUPE_REQ, MPI_COMM_WORLD, &status);
//        if (rc != MPI_SUCCESS) [[unlikely]] {
//            MPI_Abort(MPI_COMM_WORLD, rc);
//        }
//        std::cout << "after recv" << std::endl;
//
//        auto res = deduplicate ({buf.get(), static_cast <size_t> (data_size)});
//        res.second.emplace_back(0, res.first); // last element contains the effective size
//        std::cout << "dedupe effective size" << res.first << std::endl;
//        const auto size = static_cast <int> (res.second.size() * sizeof (wide_span));
//        rc = MPI_Send(res.second.data(), size, MPI_CHAR, source, message_types::DEDUPE_RESP, MPI_COMM_WORLD);
//        if (rc != MPI_SUCCESS) [[unlikely]] {
//            MPI_Abort(MPI_COMM_WORLD, rc);
//        }
//    }
//
//    std::pair <uint128_t, size_t> get_updated_comparison (const dedupe::set_data& set_elem) {
//        if (set_elem.data_offset > m_storage.m_temp_offset) { // data in cache, might split in data store
//            const auto tmp_pointer_addresses = m_storage.sync_cache ();
//            for (const auto& tmp_pointer_addr: tmp_pointer_addresses) {
//                const auto set_offset = m_cached_set_elements.at(tmp_pointer_addr.first);
//                m_fragment_set.update_pointer(set_offset, tmp_pointer_addr.second.front());
//            }
//        }
//        else {
//            return {set_elem.data_offset, set_elem.data.size};
//        }
//    }
//
//    std::pair <std::size_t, address> deduplicate (std::string_view data) {
//
//        std::pair <std::size_t, address> result;
//        auto integration_data = data;
//
//        while (!integration_data.empty()) {
//            const auto f = m_fragment_set.find(integration_data);
//            if (f.match) {
//                const auto [data_offset, data_size] = get_updated_comparison (f.match.value());
//                result.second.emplace_back(wide_span{f.match->data_offset, integration_data.size()});
//                integration_data = integration_data.substr(integration_data.size());
//                continue;
//            }
//
//            const std::string_view lower_data_str {f.lower->data.data.get(), f.lower->data.size};
//            const auto lower_common_prefix = largest_common_prefix (integration_data, lower_data_str);
//
//            if (lower_common_prefix == integration_data.size()) {
//
//                result.second.emplace_back(wide_span {f.lower->data_offset, integration_data.size()});
//                integration_data = integration_data.substr(integration_data.size());
//                continue;
//            }
//
//            const std::string_view upper_data_str {f.upper->data.data.get(), f.lower->data.size};
//            const auto upper_common_prefix = largest_common_prefix (integration_data, upper_data_str);
//            auto max_common_prefix = upper_common_prefix;
//            auto max_data_offset = f.upper->data_offset;
//            if (max_common_prefix < lower_common_prefix) {
//                max_common_prefix = lower_common_prefix;
//                max_data_offset = f.lower->data_offset;
//            }
//
//            if (max_common_prefix < m_dedupe_conf.min_fragment_size or integration_data.size() - max_common_prefix < m_dedupe_conf.min_fragment_size) {
//
//                const auto size = std::min (integration_data.size(), m_dedupe_conf.max_fragment_size);
//                const auto addr = m_storage.cache_write(integration_data.substr(0, size));
//                const auto set_elem = m_fragment_set.add_pointer (integration_data.substr(0, addr.front().size), addr.front().pointer, f.index);
//                //m_cached_set_elements.emplace(set_elem.position);
//
//                result.second.insert(result.second.cend(), addr.cbegin(), addr.cend());
//                result.first += size;
//                integration_data = integration_data.substr(size);
//                continue;
//            }
//            else if (max_common_prefix == integration_data.size()) {
//                result.second.emplace_back(wide_span {max_data_offset, integration_data.size()});
//                integration_data = integration_data.substr(integration_data.size());
//                continue;
//            }
//            else {
//                result.second.emplace_back (wide_span {max_data_offset, max_common_prefix});
//                integration_data = integration_data.substr(max_common_prefix, integration_data.size() - max_common_prefix);
//                continue;
//            }
//
//        }
//
//        m_storage.sync(result.second);
//        return result;
//    }
//
//    static size_t largest_common_prefix(const std::string_view &str1, const std::string_view &str2) noexcept {
//        size_t i = 0;
//        const auto min_size = std::min (str1.size(), str2.size());
//        while (i < min_size and str1[i] == str2[i]) {
//            i++;
//        }
//        return i;
//    }
//
//    const cluster_ranks m_cluster_plan;
//    const int m_id;
//    const std::string m_job_name;
//    dedupe_config m_dedupe_conf;
//    global_data m_storage;
//    dedupe::paged_redblack_tree <dedupe::set_full_comparator> m_fragment_set;
//    std::unordered_map <uint128_t, uint64_t> m_cached_set_elements;
//    std::atomic <bool> m_stop = false;
//
//};
//} // end namespace uh::cluster
//
//#endif //CORE_DEDUPE_JOB_H
//
// Created by masi on 7/17/23.
//

#ifndef CORE_DEDUPE_NODE_H
#define CORE_DEDUPE_NODE_H

#include <functional>
#include <iostream>
#include <utility>
#include "common/cluster_config.h"
#include "global_data.h"
#include "paged_redblack_tree.h"
#include "dedupe_node_handler.h"

namespace uh::cluster {
    class dedupe_node {
    public:

        dedupe_node (int id, cluster_map&& cmap):
                m_cluster_map (std::move (cmap)),
                m_id (id),
                m_job_name ("dedupe_node_" + std::to_string (id)),
                m_storage (m_cluster_map, m_cluster_map.m_cluster_conf.dedupe_node_conf.data_node_connection_count,
                           m_cluster_map.m_cluster_conf.dedupe_node_conf.server_conf.threads),
                m_server (m_cluster_map.m_cluster_conf.dedupe_node_conf.server_conf,
                          std::make_unique <dedupe_node_handler>(m_cluster_map.m_cluster_conf.dedupe_node_conf, m_storage))
    {

        }

        void run() {
            std::cout << "hello from " << m_job_name << std::endl;
            m_server.run();
        }


        const cluster_map m_cluster_map;
        const int m_id;
        const std::string m_job_name;
        global_data m_storage;
        server m_server;

        std::atomic <bool> m_stop = false;

    };
} // end namespace uh::cluster

#endif //CORE_DEDUPE_NODE_H