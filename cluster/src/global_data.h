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
                          m_data_nodes (data_node_ranks)
                          //, m_temp_offset (temp_offset_start)
    {

        MPI_Status status;
        int rc;

        for (const auto rank: data_node_ranks) {
            int id;
            rc = MPI_Send (&rank, 1, MPI_INT, rank, message_types::INIT_REQ, MPI_COMM_WORLD);
            if (rc != MPI_SUCCESS) [[unlikely]] {
                throw std::runtime_error ("Could not send init request");
            }
            rc = MPI_Recv(&id, 1, MPI_INT, rank, message_types::INIT_RESP, MPI_COMM_WORLD, &status);
            if (rc != MPI_SUCCESS) [[unlikely]] {
                throw std::runtime_error ("Could not receive init response");
            }
            m_data_node_offsets.emplace (conf.max_data_store_size * id, rank);
        }

        int rank;
        rc = MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        if (rc != MPI_SUCCESS) [[unlikely]] {
            throw std::runtime_error ("Could not fetch the rank");
        }
        rank_index = rank % m_data_nodes.size();
    }

    address write (const std::string_view& data) {
        const auto source = m_data_nodes [rank_index];
        rank_index = (rank_index + 1) % m_data_nodes.size();
        auto rc = MPI_Send(data.data(), static_cast <int> (data.size()), MPI_CHAR, source, message_types::WRITE_REQ, MPI_COMM_WORLD);
        if (rc != MPI_SUCCESS) [[unlikely]] {
            throw std::runtime_error("Could not send the data to be written");
        }

        MPI_Status status;
        rc = MPI_Probe (source, message_types::WRITE_RESP, MPI_COMM_WORLD, &status);
        if (rc != MPI_SUCCESS) [[unlikely]] {
            throw std::runtime_error("Could not get the address size of the sent data");
        }
        int address_size;
        MPI_Get_count (&status, MPI_CHAR, &address_size);
        address addr (address_size / sizeof (wide_span));
        rc = MPI_Recv (addr.data(), address_size, MPI_CHAR, source, message_types::WRITE_REQ, MPI_COMM_WORLD, &status);
        if (rc != MPI_SUCCESS) [[unlikely]] {
            throw std::runtime_error("Could not receive the address of the sent data");
        }

        return addr;
    }

    /*
    address cache_write (const std::string_view& data) {
        m_temp_offset = m_temp_offset - 1;
        m_cache.emplace(m_temp_offset, data);
        return {wide_span {m_temp_offset, data.size()}};
    }
     */

    std::size_t read (char* buffer, const uint128_t pointer, const size_t size) const {
        const auto source = get_data_node (pointer);
        address addr {{pointer, size}};
        auto rc = MPI_Send(addr.data(), static_cast <int> (addr.size()), MPI_CHAR, source, message_types::READ_REQ, MPI_COMM_WORLD);
        if (rc != MPI_SUCCESS) [[unlikely]] {
            throw std::runtime_error("Could not send the read request");
        }

        MPI_Status status;
        rc = MPI_Probe (source, message_types::READ_RESP, MPI_COMM_WORLD, &status);
        if (rc != MPI_SUCCESS) [[unlikely]] {
            throw std::runtime_error("Could not get the data size of the read response");
        }
        int data_size;
        MPI_Get_count (&status, MPI_CHAR, &data_size);
        rc = MPI_Recv (buffer, data_size, MPI_CHAR, source, message_types::READ_RESP, MPI_COMM_WORLD, &status);
        if (rc != MPI_SUCCESS) [[unlikely]] {
            throw std::runtime_error("Could not get the data of the read response");
        }

        return data_size;
    }

    void remove (const uint128_t pointer, const size_t size) {
        const auto source = get_data_node (pointer);
        address addr {{pointer, size}};
        auto rc = MPI_Send(addr.data(), static_cast <int> (addr.size()), MPI_CHAR, source, message_types::REMOVE_REQ, MPI_COMM_WORLD);
        if (rc != MPI_SUCCESS) [[unlikely]] {
            throw std::runtime_error("Could not send the remove request");
        }

        MPI_Status status;
        int dummy;
        rc = MPI_Recv (&dummy, 1, MPI_INT, source, message_types::REMOVE_OK, MPI_COMM_WORLD, &status);
        if (rc != MPI_SUCCESS) [[unlikely]] {
            throw std::runtime_error("Could not get the remove confirmation");
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
                throw std::runtime_error("Could not send the sync request");
            }

            MPI_Status status;
            rc = MPI_Recv (&dummy, 1, MPI_INT, target, message_types::SYNC_OK, MPI_COMM_WORLD, &status);
            if (rc != MPI_SUCCESS) [[unlikely]] {
                throw std::runtime_error("Could not get the sync confirmation");
            }
        }

    }

    [[nodiscard]] uint128_t get_used_space () const {
        uint128_t total_used = 0;
        for (const auto target: m_data_nodes) {
            int dummy;
            auto rc = MPI_Send(&dummy, 1, MPI_INT, target, message_types::USED_REQ, MPI_COMM_WORLD);
            if (rc != MPI_SUCCESS) [[unlikely]] {
                throw std::runtime_error("Could not send the get_used request");
            }

            MPI_Status status;
            uint64_t buffer [2];
            rc = MPI_Recv (buffer, 2, MPI_UINT64_T, target, message_types::USED_RESP, MPI_COMM_WORLD, &status);
            if (rc != MPI_SUCCESS) [[unlikely]] {
                throw std::runtime_error("Could not get the used space information");
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
private:

    [[nodiscard]] int get_data_node (const uint128_t& pointer) const {
        const auto pfd = m_data_node_offsets.upper_bound (pointer);
        if (pfd == m_data_node_offsets.cbegin()) [[unlikely]] {
            throw std::out_of_range ("The given data pointer could not be found among the available data nodes");
        }
        return std::prev (pfd)->second;
    }

    //std::unordered_map <uint128_t, std::string_view> m_cache;
    //uint128_t m_temp_offset;
    //constexpr static uint128_t temp_offset_start = uint128_t (std::numeric_limits <uint64_t>::max(), std::numeric_limits <uint64_t>::max());
    std::map <const uint128_t, const int> m_data_node_offsets;
    std::vector <int> m_data_nodes;
    std::size_t rank_index {};

};

} // end namespace uh::cluster

#endif //CORE_GLOBAL_DATA_H
