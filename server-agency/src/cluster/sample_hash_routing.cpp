//
// Created by masi on 4/19/23.
//

#include "sample_hash_routing.h"

namespace uh::an::cluster {

sample_hash_routing::sample_hash_routing(
        const std::unordered_map<std::string, std::unique_ptr<protocol::client_pool>> &nodes) :
        m_nodes (nodes),
        m_nodes_index(fill_node_index (m_nodes)){}

const std::unique_ptr<protocol::client_pool> &sample_hash_routing::route_data(const std::span <char> &data) const {


    std::string sample;

    if (data.size() < 64) {
        sample.append(data.data(), data.size());
    }
    else {
        sample.reserve(64);

        const std::size_t step = data.size() / 4;
        auto data_ptr = data.data();

        sample.append(data_ptr, 16);
        data_ptr += step;
        sample.append(data_ptr, 16);
        data_ptr += step;
        sample.append(data_ptr, 16);
        data_ptr += step;
        sample.append(data_ptr, 16);
    }
    
    const auto hash = std::hash <std::string> {} (sample);

    return m_nodes_index.at(hash % m_nodes_index.size());
}

sample_hash_routing::node_index_t sample_hash_routing::fill_node_index(
        const std::unordered_map<std::string, std::unique_ptr<protocol::client_pool>> &nodes) {
    node_index_t nodes_index;

    size_t index = 0;
    for (const auto& node_pair : nodes) {
        nodes_index.emplace(index++, std::cref (node_pair.second));
    }
    return nodes_index;
}

}