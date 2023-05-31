//
// Created by masi on 4/19/23.
//

#include "sample_hash_routing.h"

namespace uh::an::cluster {

sample_hash_routing::sample_hash_routing(
        const std::unordered_map<std::string, std::unique_ptr<protocol::client_pool>>& nodes) :
        m_nodes (nodes),
        m_nodes_index(fill_node_index (m_nodes)){
}

// ---------------------------------------------------------------------

protocol::client_pool& sample_hash_routing::route_data(const std::span <const char>& data) const {


    char sample [SAMPLE_SIZE];

    if (data.size() < SAMPLE_SIZE) {
        std::memcpy (sample, data.data(), data.size());
        std::memset (sample, 0, SAMPLE_SIZE - data.size());
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

routing_interface::db_hash_offsets_map sample_hash_routing::route_hashes(const std::span<char> &hashes) const {
    db_hash_offsets_map res;
    std::list <size_t> offsets;
    for (size_t i = 0; i < hashes.size(); i+=64) {
        offsets.push_back(i);
    }
    res [&m_nodes_index.at (0)] = std::move (offsets);

    return res;
}

// ---------------------------------------------------------------------

sample_hash_routing::node_index_t sample_hash_routing::fill_node_index(
        const std::unordered_map<std::string, std::unique_ptr<protocol::client_pool>> &nodes) {
    node_index_t nodes_index;

    size_t index = 0;
    for (const auto& node_pair : nodes) {
        nodes_index.emplace(index++, *node_pair.second);
    }
    return nodes_index;
}

// ---------------------------------------------------------------------

const std::hash<std::string> &sample_hash_routing::get_hash_func() {
    const static std::hash <std::string> hash_func {};
    return hash_func;
}

// ---------------------------------------------------------------------

} // namespace uh::an::cluster