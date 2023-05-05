//
// Created by masi on 4/19/23.
//

#ifndef CORE_ROUTING_INTERFACE_H
#define CORE_ROUTING_INTERFACE_H

#include <unordered_map>

namespace uh::an::cluster
{

struct routing_interface {

    using db_hash_offsets_map = std::unordered_map <uh::protocol::client_pool*, std::list <size_t>>;

    [[nodiscard]] virtual protocol::client_pool &route_data (const std::span <const char> &data) const = 0;

    [[nodiscard]] virtual db_hash_offsets_map route_hashes (const std::span <char> &hashes) const = 0;


    virtual ~routing_interface() = default;

};

}

#endif //CORE_ROUTING_INTERFACE_H
