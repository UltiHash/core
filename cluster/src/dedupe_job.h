//
// Created by masi on 7/17/23.
//

#ifndef CORE_DEDUPE_JOB_H
#define CORE_DEDUPE_JOB_H

#include <functional>
#include <iostream>
#include "cluster_config.h"
#include "global_data.h"
#include "paged_redblack_tree.h"

namespace uh::cluster {
class dedupe_job {
public:

    dedupe_job (int id, dedupe_config conf, cluster_ranks cluster_plan):
            m_cluster_plan (std::move (cluster_plan)),
            m_id (id),
            m_job_name ("dedupe_node_" + std::to_string (id)),
            m_dedupe_conf(conf),
            m_storage (m_dedupe_conf.storage_conf, cluster_plan.data_node_ranks),
            m_fragment_set (set_config{}, m_storage) {

    }

    void run() {
        std::cout << "hello from " << m_job_name << std::endl;

        MPI_Status status;
        int message_size;

        while (!m_stop) {

            MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

            try {
                switch (status.MPI_TAG) {
                    case message_types::DEDUPE:
                        MPI_Get_count (&status, MPI_CHAR, &message_size);
                        handle_dedupe (status.MPI_SOURCE, message_size);
                        break;
                    default:
                        throw std::invalid_argument("Unknown tag");
                }
            }
            catch (std::exception& e) {
                handle_failure (m_job_name, status.MPI_SOURCE, e);
            }
        }
    }

    void handle_dedupe (int source, int data_size) {
        MPI_Status status;
        const auto buf = std::make_unique_for_overwrite<char []> (data_size);
        auto rc = MPI_Recv(buf.get(), data_size, MPI_CHAR, source, message_types::DEDUPE, MPI_COMM_WORLD, &status);
        if (rc != MPI_SUCCESS) [[unlikely]] {
            throw std::runtime_error("Could not receive the data for deduplication");
        }

        auto res = deduplicate ({buf.get(), static_cast <size_t> (data_size)});
        res.second.emplace_back(0, res.first); // last element contains the effective size
        const auto size = static_cast <int> (res.second.size() * sizeof (wide_span));
        rc = MPI_Send(res.second.data(), size, MPI_CHAR, source, message_types::WRITE_RESP, MPI_COMM_WORLD);
        if (rc != MPI_SUCCESS) [[unlikely]] {
            throw std::runtime_error("Could not send the address of the written data");
        }
    }

    std::pair <std::size_t, address> deduplicate (std::string_view data) {

        auto total_data = data;
        std::pair <std::size_t, address> result;
        std::size_t dangling_offset = 0;

        auto integration_data = total_data;
        // TODO every continue command, i.e. every time we find duplicates, we need to write also the dangling data
        while (!integration_data.empty()) {
            const auto f = m_fragment_set.find({integration_data.data(), integration_data.size()});
            if (f.match) {
                result.second.emplace_back(wide_span{f.match->data_offset, integration_data.size()});
                integration_data = integration_data.substr(integration_data.size());
                continue;
            }

            const std::string_view lower_data_str {f.lower->data.data.get(), f.lower->data.size};
            const auto lower_common_prefix = largest_common_prefix (integration_data, lower_data_str);

            if (lower_common_prefix == integration_data.size()) {
                m_fragment_set.add_pointer (integration_data, f.lower->data_offset, f.index);
                result.second.emplace_back(wide_span {f.lower->data_offset, integration_data.size()});
                integration_data = integration_data.substr(integration_data.size());
                continue;
            }

            const std::string_view upper_data_str {f.upper->data.data.get(), f.lower->data.size};
            const auto upper_common_prefix = largest_common_prefix (integration_data, upper_data_str);
            auto max_common_prefix = upper_common_prefix;
            auto max_data_offset = f.upper->data_offset;
            if (max_common_prefix < lower_common_prefix) {
                max_common_prefix = lower_common_prefix;
                max_data_offset = f.lower->data_offset;
            }

            if (max_common_prefix < m_dedupe_conf.min_fragment_size or integration_data.size() - max_common_prefix < m_dedupe_conf.min_fragment_size) {

                const auto size = std::min (total_data.size(), m_dedupe_conf.max_fragment_size);
                if (dangling_offset >= size) {
                    const auto offset = store_data(total_data.substr(0, size));
                    m_fragment_set.add_pointer (integration_data.substr(0, size), offset, f.index);
                    result.second.emplace_back (wide_span {offset, size});
                    result.first += size;
                    total_data = total_data.substr(size);
                    dangling_offset = 0;
                }
                else {
                    dangling_offset += m_dedupe_conf.sampling_interval;
                }
                integration_data = total_data.substr(dangling_offset);

                continue;
            }
            else if (max_common_prefix == integration_data.size()) {
                m_fragment_set.add_pointer (integration_data, max_data_offset, f.index);
                result.second.emplace_back(wide_span {max_data_offset, integration_data.size()});
                integration_data = integration_data.substr(integration_data.size());
                continue;
            }
            else {
                m_fragment_set.add_pointer (integration_data.substr(0, max_common_prefix), max_data_offset, f.index);
                result.second.emplace_back (wide_span {max_data_offset, max_common_prefix});
                integration_data = integration_data.substr(max_common_prefix, integration_data.size() - max_common_prefix);
                continue;
            }




        }




        //auto integration_data = data;
        //std::pair <std::size_t, address> result;


        while (!integration_data.empty()) {
            const auto f = m_fragment_set.find({integration_data.data(), integration_data.size()});
            if (f.match) {
                result.second.emplace_back(wide_span{f.match->data_offset, integration_data.size()});
                integration_data = integration_data.substr(integration_data.size());
                continue;
            }

            const std::string_view lower_data_str {f.lower->data.data.get(), f.lower->data.size};
            const auto lower_common_prefix = largest_common_prefix (integration_data, lower_data_str);

            if (lower_common_prefix == integration_data.size()) {
                m_fragment_set.add_pointer (integration_data, f.lower->data_offset, f.index);
                result.second.emplace_back(wide_span {f.lower->data_offset, integration_data.size()});
                integration_data = integration_data.substr(integration_data.size());
                continue;
            }

            const std::string_view upper_data_str {f.upper->data.data.get(), f.lower->data.size};
            const auto upper_common_prefix = largest_common_prefix (integration_data, upper_data_str);
            auto max_common_prefix = upper_common_prefix;
            auto max_data_offset = f.upper->data_offset;
            if (max_common_prefix < lower_common_prefix) {
                max_common_prefix = lower_common_prefix;
                max_data_offset = f.lower->data_offset;
            }

            if (max_common_prefix < m_dedupe_conf.min_fragment_size or integration_data.size() - max_common_prefix < m_dedupe_conf.min_fragment_size) {
                const auto size = std::min (integration_data.size(), m_dedupe_conf.max_fragment_size);
                const auto offset = store_data(integration_data.substr(0, size));
                m_fragment_set.add_pointer (integration_data.substr(0, size), offset, f.index);
                result.second.emplace_back(wide_span {offset, size});
                result.first += size;
                integration_data = integration_data.substr(size);
                continue;
            }
            else if (max_common_prefix == integration_data.size()) {
                m_fragment_set.add_pointer (integration_data, max_data_offset, f.index);
                result.second.emplace_back(wide_span {max_data_offset, integration_data.size()});
                integration_data = integration_data.substr(integration_data.size());
                continue;
            }
            else {
                m_fragment_set.add_pointer (integration_data.substr(0, max_common_prefix), max_data_offset, f.index);
                result.second.emplace_back (wide_span {max_data_offset, max_common_prefix});
                integration_data = integration_data.substr(max_common_prefix, integration_data.size() - max_common_prefix);
                continue;
            }
        }

        return result;
    }

    static size_t largest_common_prefix(const std::string_view &str1, const std::string_view &str2) noexcept {
        size_t i = 0;
        const auto min_size = std::min (str1.size(), str2.size());
        while (i < min_size and str1[i] == str2[i]) {
            i++;
        }
        return i;
    }

    uint128_t store_data(const std::string_view& frag) {

    }

    const cluster_ranks m_cluster_plan;
    const int m_id;
    const std::string m_job_name;
    dedupe_config m_dedupe_conf;
    global_data m_storage;
    dedupe::paged_redblack_tree <dedupe::set_full_comparator> m_fragment_set;

    std::atomic <bool> m_stop = false;

};
} // end namespace uh::cluster

#endif //CORE_DEDUPE_JOB_H
