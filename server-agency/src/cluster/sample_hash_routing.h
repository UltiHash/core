//
// Created by masi on 4/19/23.
//

#ifndef CORE_SAMPLE_HASH_ROUTING_H
#define CORE_SAMPLE_HASH_ROUTING_H

#include <memory>
#include <unordered_map>
#include <functional>
#include <protocol/client_pool.h>
#include "routing_interface.h"
#include <util/exception.h>

namespace uh::an::cluster
{
    class sample_hash_routing: public routing_interface {
    public:
        explicit sample_hash_routing(const std::unordered_map<std::string, std::unique_ptr<protocol::client_pool>> &nodes);

        protocol::client_pool &route_data (const std::span <const char>& data) const override;

        [[nodiscard]] db_hash_offsets_map route_hashes (const std::span <char> &hashes) const override;

    private:

        using node_index_t = std::unordered_map<std::size_t, protocol::client_pool&>;

        static node_index_t
        fill_node_index (const std::unordered_map<std::string, std::unique_ptr<protocol::client_pool>> &);

        const std::unordered_map<std::string, std::unique_ptr<protocol::client_pool>> &m_nodes;
        const node_index_t m_nodes_index;
        static constexpr std::size_t SAMPLE_SIZE = 64;
        static constexpr std::size_t SAMPLE_PIECES = 4;
        static_assert (SAMPLE_SIZE % SAMPLE_PIECES == 0);

        static const std::hash <std::string> &get_hash_func ();
        const std::hash <std::string> &hash_func = get_hash_func();
    };

}
#endif //CORE_SAMPLE_HASH_ROUTING_H
