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
    explicit global_data (global_data_config conf,
                          const std::vector <int>& data_node_ranks):
                          m_data_nodes (data_node_ranks),
                          m_temp_offset (temp_offset_start)
    {

        std::vector <MPI_Request> reqs (data_node_ranks.size());
        std::vector <MPI_Status> stats (data_node_ranks.size());

        int rc;

        for (int i = 0; i < data_node_ranks.size(); ++i) {
            rc = MPI_Isend (&data_node_ranks[i], 1, MPI_INT, data_node_ranks[i], message_types::INIT_REQ, MPI_COMM_WORLD, &reqs[i]);
            if (rc != MPI_SUCCESS) [[unlikely]] {
                MPI_Abort(MPI_COMM_WORLD, rc);
            }
        }

        MPI_Waitall(static_cast <int> (data_node_ranks.size()), reqs.data(), stats.data());

        for (int i = 0; i < data_node_ranks.size(); ++i) {
            int id;
            rc = MPI_Recv(&id, 1, MPI_INT, data_node_ranks[i], message_types::INIT_RESP, MPI_COMM_WORLD, &stats[i]);
            if (rc != MPI_SUCCESS) [[unlikely]] {
                MPI_Abort(MPI_COMM_WORLD, rc);
            }
            m_data_node_offsets.emplace (conf.max_data_store_size * id, data_node_ranks[i]);
        }

        int rank;
        rc = MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        if (rc != MPI_SUCCESS) [[unlikely]] {
            MPI_Abort(MPI_COMM_WORLD, rc);
        }
        rank_index = rank % m_data_nodes.size();
    }

    address write (const std::string_view& data) {
        const auto source = m_data_nodes [rank_index];
        rank_index = (rank_index + 1) % m_data_nodes.size();
        auto rc = MPI_Send(data.data(), static_cast <int> (data.size()), MPI_CHAR, source, message_types::WRITE_REQ, MPI_COMM_WORLD);
        if (rc != MPI_SUCCESS) [[unlikely]] {
            MPI_Abort(MPI_COMM_WORLD, rc);
        }

        MPI_Status status;
        rc = MPI_Probe (source, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        if (rc != MPI_SUCCESS) [[unlikely]] {
            MPI_Abort(MPI_COMM_WORLD, rc);
        }
        int message_size;

        MPI_Get_count (&status, MPI_CHAR, &message_size);

        if (status.MPI_TAG != message_types::WRITE_RESP) [[unlikely]] {
            ospan<char> buffer (message_size);
            rc = MPI_Recv (buffer.data.get(), message_size, MPI_CHAR, source, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            if (rc != MPI_SUCCESS) [[unlikely]] {
                MPI_Abort(MPI_COMM_WORLD, rc);
            }
            throw std::runtime_error (std::string (buffer.data.get(), buffer.size));
        }
        address addr (message_size / sizeof (wide_span));
        rc = MPI_Recv (addr.data(), message_size, MPI_CHAR, source, message_types::WRITE_RESP, MPI_COMM_WORLD, &status);
        if (rc != MPI_SUCCESS) [[unlikely]] {
            MPI_Abort(MPI_COMM_WORLD, rc);
        }

        return addr;
    }


    address cache_write (const std::string_view& data) {
        m_temp_offset = m_temp_offset - 1;
        m_cache.emplace(m_temp_offset, data);
        return {wide_span {m_temp_offset, data.size()}};
    }

    std::unordered_map <uint128_t, address> sync_cache () {

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
    }


    std::size_t read (char* buffer, const uint128_t pointer, const size_t size) const {
        const auto source = get_data_node (pointer);
        address addr {{pointer, size}};
        auto rc = MPI_Send(addr.data(), static_cast <int> (addr.size()), MPI_CHAR, source, message_types::READ_REQ, MPI_COMM_WORLD);
        if (rc != MPI_SUCCESS) [[unlikely]] {
            MPI_Abort(MPI_COMM_WORLD, rc);
        }

        MPI_Status status;
        rc = MPI_Probe (source, message_types::READ_RESP, MPI_COMM_WORLD, &status);
        if (rc != MPI_SUCCESS) [[unlikely]] {
            MPI_Abort(MPI_COMM_WORLD, rc);
        }
        int data_size;
        MPI_Get_count (&status, MPI_CHAR, &data_size);
        rc = MPI_Recv (buffer, data_size, MPI_CHAR, source, message_types::READ_RESP, MPI_COMM_WORLD, &status);
        if (rc != MPI_SUCCESS) [[unlikely]] {
            MPI_Abort(MPI_COMM_WORLD, rc);
        }

        return data_size;
    }

    void remove (const uint128_t pointer, const size_t size) {
        const auto source = get_data_node (pointer);
        address addr {{pointer, size}};
        auto rc = MPI_Send(addr.data(), static_cast <int> (addr.size()), MPI_CHAR, source, message_types::REMOVE_REQ, MPI_COMM_WORLD);
        if (rc != MPI_SUCCESS) [[unlikely]] {
            MPI_Abort(MPI_COMM_WORLD, rc);
        }

        MPI_Status status;
        int dummy;
        rc = MPI_Recv (&dummy, 1, MPI_INT, source, message_types::REMOVE_OK, MPI_COMM_WORLD, &status);
        if (rc != MPI_SUCCESS) [[unlikely]] {
            MPI_Abort(MPI_COMM_WORLD, rc);
        }

    }

    void sync (const address& addr) {

        std::set <int> targets;
        for (const auto pointer_span: addr) {
            targets.emplace (get_data_node (pointer_span.pointer));
        }

        for (const auto target: targets) {
            int dummy;
            auto rc = MPI_Send(&dummy, 1, MPI_INT, target, message_types::SYNC_REQ, MPI_COMM_WORLD);
            if (rc != MPI_SUCCESS) [[unlikely]] {
                MPI_Abort(MPI_COMM_WORLD, rc);
            }

            MPI_Status status;
            rc = MPI_Recv (&dummy, 1, MPI_INT, target, message_types::SYNC_OK, MPI_COMM_WORLD, &status);
            if (rc != MPI_SUCCESS) [[unlikely]] {
                MPI_Abort(MPI_COMM_WORLD, rc);
            }
        }

    }

    [[nodiscard]] uint128_t get_used_space () const {
        uint128_t total_used = 0;
        for (const auto target: m_data_nodes) {
            int dummy;
            auto rc = MPI_Send(&dummy, 1, MPI_INT, target, message_types::USED_REQ, MPI_COMM_WORLD);
            if (rc != MPI_SUCCESS) [[unlikely]] {
                MPI_Abort(MPI_COMM_WORLD, rc);
            }

            MPI_Status status;
            uint64_t buffer [2];
            rc = MPI_Recv (buffer, 2, MPI_UINT64_T, target, message_types::USED_RESP, MPI_COMM_WORLD, &status);
            if (rc != MPI_SUCCESS) [[unlikely]] {
                MPI_Abort(MPI_COMM_WORLD, rc);
            }
            total_used += uint128_t (buffer[0], buffer[1]);
        }

        return total_used;

    }

    void stop () const {
        for (const auto target: m_data_nodes) {
            int dummy;
            MPI_Send(&dummy, 1, MPI_INT, target, message_types::STOP, MPI_COMM_WORLD);
        }
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
