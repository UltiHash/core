//
// Created by masi on 7/27/23.
//

#ifndef CORE_GLOBAL_DATA_H
#define CORE_GLOBAL_DATA_H

#include <map>
#include <unordered_map>
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
        int source = m_data_nodes [rank_index];
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
        return {};
    }

    void remove (const uint128_t pointer, const size_t size) {}

    void sync (const address&) {}

    [[nodiscard]] uint128_t get_used_space () const noexcept {
        return {};
    }


private:
    static std::map<uint128_t, int> map_offset_data_node(const std::vector<int> &data_node_ranks) {
        const auto first_rank = data_node_ranks.front();

        return {};
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
