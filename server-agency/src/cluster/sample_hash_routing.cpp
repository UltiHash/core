//
// Created by masi on 4/19/23.
//

#include "sample_hash_routing.h"

namespace uh::an::cluster {

sample_hash_routing::sample_hash_routing(
        const std::unordered_map<std::string, std::unique_ptr<protocol::client_pool>> &nodes) :
        m_nodes (nodes) {}

const std::unique_ptr<protocol::client_pool> &sample_hash_routing::route_data(const std::span <char> &data) {
    // TODO __backend
    return m_nodes.begin()->second;
}

}