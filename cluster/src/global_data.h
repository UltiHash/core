//
// Created by masi on 7/27/23.
//

#ifndef CORE_GLOBAL_DATA_H
#define CORE_GLOBAL_DATA_H

#include <map>
#include "data_node.h"

namespace uh::cluster {

class global_data {

public:
    explicit global_data (const uint128_t max_data_store_size, const std::vector <int>& data_node_ranks) {

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
            m_data_node_ranks.emplace (max_data_store_size * id, rank);
        }
    }

    address write (const std::span <const char> data) {}

    std::size_t read (char* buffer, const uint128_t pointer, const size_t size) const {};

    void remove (const uint128_t pointer, const size_t size) {};

    void sync (const address&) {};

    [[nodiscard]] uint128_t get_used_space () const noexcept {};


private:
    static std::map<uint128_t, int> map_offset_data_node(const std::vector<int> &data_node_ranks) {
        const auto first_rank = data_node_ranks.front();

        return {};
    }

    std::map <const uint128_t, const int> m_data_node_ranks;
};

} // end namespace uh::cluster

#endif //CORE_GLOBAL_DATA_H
