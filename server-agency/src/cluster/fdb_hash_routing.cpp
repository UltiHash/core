//
// Created by max on 31.05.23.
//

#include "fdb_hash_routing.h"

namespace uh::an::cluster {

    fdb_hash_routing::fdb_hash_routing(
            const std::unordered_map<std::string, std::unique_ptr<protocol::client_pool>>& nodes) :
            m_nodes (nodes),
            m_nodes_index(fill_node_index (m_nodes)){
    }

// ---------------------------------------------------------------------

    // route_data copied from sample_hash_routing.cpp, should be replaced with something more
    // elaborate e.g. locality preserving hashes for co-locating similar data on the same data nodes
    // + load balancing
    protocol::client_pool& fdb_hash_routing::route_data(const std::span <const char>& data) const {


        char sample [SAMPLE_SIZE];

        if (data.size() < SAMPLE_SIZE) {
            std::memcpy (sample, data.data(), data.size());
            std::memset (sample +  data.size(), 0, SAMPLE_SIZE - data.size());
        }
        else {

            const std::size_t step = data.size() / SAMPLE_PIECES;
            constexpr auto sample_piece_size = SAMPLE_SIZE / SAMPLE_PIECES;

            auto data_ptr = data.data();
            auto sample_ptr = sample;

            for (auto i = 0; i < SAMPLE_PIECES; ++i) {
                std::memcpy (sample_ptr, data_ptr, sample_piece_size);
                data_ptr += step;
                sample_ptr += sample_piece_size;
            }
        }

        const auto hash = hash_func ({sample, SAMPLE_SIZE});

        return m_nodes_index.at (hash % m_nodes_index.size());
    }

// ---------------------------------------------------------------------

    routing_interface::db_hash_offsets_map fdb_hash_routing::route_hashes(const std::span<char> &hashes) const {
        auto fdb = uh::fdb::fdb("/etc/foundationdb/fdb.cluster");
        // TODO: mark transaction as snapshot read for improved performance
        auto trans = fdb.make_transaction();
        std::unordered_map<std::string, std::list<std::size_t>> offsets_by_uuid;
        db_hash_offsets_map res;
        std::list <size_t> offsets;
        for (size_t i = 0; i < hashes.size(); i+=64) {
            offsets.push_back(i);
            auto hash = hashes.subspan(i, 64);
            auto db_res = trans->get(hash);
            if(db_res.has_value()) {
                std::string uuid(db_res.value().value.begin(), db_res.value().value.end());
                protocol::client_pool& pool = *m_nodes.at(uuid);
                if(offsets_by_uuid.contains(pool.get_server_uuid())) {
                    offsets_by_uuid.at(pool.get_server_uuid()).push_back(i);
                } else {
                    offsets_by_uuid.emplace(std::make_pair(uuid, std::list<std::size_t>{i}));
                }
            }
        }

        // TODO: the offset by client_pool map structure needs to be replaced with something cleaner,
        // e.g. offset by data node uuid
        for(auto node : offsets_by_uuid) {
            res [&*m_nodes.at(node.first)] = std::move (node.second);
        }

        return res;
    }

// ---------------------------------------------------------------------

    fdb_hash_routing::node_index_t fdb_hash_routing::fill_node_index(
            const std::unordered_map<std::string, std::unique_ptr<protocol::client_pool>> &nodes) {
        node_index_t nodes_index;

        size_t index = 0;
        for (const auto& node_pair : nodes) {
            nodes_index.emplace(index++, *node_pair.second);
        }
        return nodes_index;
    }

// ---------------------------------------------------------------------

    const std::hash<std::string> &fdb_hash_routing::get_hash_func() {
        const static std::hash <std::string> hash_func {};
        return hash_func;
    }
}