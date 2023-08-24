//
// Created by masi on 7/27/23.
//

#ifndef CORE_GLOBAL_DATA_H
#define CORE_GLOBAL_DATA_H

#include <map>
#include <unordered_map>
#include <set>
#include "data_node.h"


namespace uh::cluster {


class global_data {


public:

    explicit global_data (global_data_config conf):
                          m_temp_offset (temp_offset_start)
    {

    }

    address write (const std::string_view& data) {
        return {};
    }


    address cache_write (const std::string_view& data) {
        m_temp_offset = m_temp_offset - 1;
        m_cache.emplace(m_temp_offset, data);
        return {wide_span {m_temp_offset, data.size()}};
    }

    std::unordered_map <uint128_t, address> sync_cache () {
        return {};
        /*
        if (m_temp_offset == temp_offset_start) [[unlikely]] {
            return {};
        }

        std::vector <MPI_Request> reqs (m_data_nodes.size());
        std::vector <MPI_Status> stats (m_data_nodes.size());

        const auto total_size = (temp_offset_start - m_temp_offset).get_data()[1];
        auto buf = std::make_unique_for_overwrite <char[]> (total_size);

        std::size_t offset = 0;
        for (const auto& cached: m_cache) {
            std::memcpy(buf.get() + offset, cached.second.data(), cached.second.size());
            offset += cached.second.size();
        }

        const int portion = static_cast <int> (total_size / m_data_nodes.size());
        int rc;
        for (int i = 0; i < m_data_nodes.size(); ++i) {
            rc = MPI_Isend (buf.get() + i * portion, portion, MPI_CHAR, m_data_nodes[i], message_types::WRITE_REQ, MPI_COMM_WORLD, &reqs[i]);
            if (rc != MPI_SUCCESS) [[unlikely]] {
                MPI_Abort(MPI_COMM_WORLD, rc);
            }
        }

        MPI_Waitall(static_cast <int> (m_data_nodes.size()), reqs.data(), stats.data());

        size_t total_response_size = 0;
        std::vector <int> message_sizes (m_data_nodes.size());
        for (int i = 0; i < m_data_nodes.size(); ++i) {
            rc = MPI_Probe(MPI_ANY_SOURCE, message_types::WRITE_RESP, MPI_COMM_WORLD, &stats[i]);
            if (rc != MPI_SUCCESS) [[unlikely]] {
                MPI_Abort(MPI_COMM_WORLD, rc);
            }

            MPI_Get_count(&stats[i], MPI_CHAR, &message_sizes[i]);
            total_response_size += message_sizes[i];
        }

        address addresses (total_response_size);

        offset = 0;
        for (int i = 0; i < m_data_nodes.size(); ++i) {
            rc = MPI_Irecv (reinterpret_cast <char *> (addresses.data()) + offset, portion, MPI_CHAR, m_data_nodes[i], message_types::WRITE_RESP, MPI_COMM_WORLD, &reqs[i]);
            if (rc != MPI_SUCCESS) [[unlikely]] {
                MPI_Abort(MPI_COMM_WORLD, rc);
            }
            offset += message_sizes[i];
        }

        MPI_Waitall(static_cast <int> (m_data_nodes.size()), reqs.data(), stats.data());

        std::unordered_map <uint128_t, address> result;
        std::size_t i = 0;
        for (const auto& cached: m_cache) {
            std::size_t size = 0;
            address addr;
            while (size < cached.second.size()) {
                addr.emplace_back(addresses[i]);
                size += addresses[i].size;
            }
            if (size != cached.second.size()) [[unlikely]] {
                throw std::logic_error ("Size mismatch when receiving the address of the written data");
            }
            result.emplace(cached.first, std::move (addr));
        }

        m_cache.clear();
        m_temp_offset = temp_offset_start;

        return result;
         */
    }


    std::size_t read (char* buffer, const uint128_t pointer, const size_t size) const {
        return {};
    }

    void remove (const uint128_t pointer, const size_t size) {
    }

    void sync (const address& addr) {

    }

    [[nodiscard]] uint128_t get_used_space () const {
        return {};
    }

    void stop () const {

    }

    uint128_t m_temp_offset;

private:

    [[nodiscard]] int get_data_node (const uint128_t& pointer) const {
        const auto pfd = m_data_node_offsets.upper_bound (pointer);
        if (pfd == m_data_node_offsets.cbegin()) [[unlikely]] {
            throw std::out_of_range ("The given data pointer could not be found among the available data nodes");
        }
        return std::prev (pfd)->second;
    }

    std::unordered_map <uint128_t, std::string_view> m_cache;
    constexpr static uint128_t temp_offset_start = uint128_t (std::numeric_limits <uint64_t>::max(), std::numeric_limits <uint64_t>::max());
    std::map <const uint128_t, const int> m_data_node_offsets;
    std::vector <int> m_data_nodes;
    std::size_t rank_index {};

};

} // end namespace uh::cluster

#endif //CORE_GLOBAL_DATA_H
