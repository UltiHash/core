//
// Created by masi on 4/19/23.
//

#ifndef CORE_SAMPLE_HASH_ROUTING_H
#define CORE_SAMPLE_HASH_ROUTING_H

#include <memory>
#include <unordered_map>
#include <protocol/client_pool.h>
#include "routing_interface.h"

namespace uh::an::cluster
{
    class sample_hash_routing: public routing_interface {
    public:
        explicit sample_hash_routing(const std::unordered_map<std::string, std::unique_ptr<protocol::client_pool>> &nodes);

        const std::unique_ptr<protocol::client_pool> &route_data (const std::span <char> &data) override;

    private:
        const std::unordered_map<std::string, std::unique_ptr<protocol::client_pool>> &m_nodes;
    };

}
#endif //CORE_SAMPLE_HASH_ROUTING_H
