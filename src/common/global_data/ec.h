//
// Created by masi on 10/17/23.
//

#ifndef CORE_EC_H
#define CORE_EC_H

#include "common/network/client.h"
#include <memory>
#include <ranges>


namespace uh::cluster {
    struct ec {
        [[nodiscard]] virtual int get_acquired_ec_node_count () const = 0;
        [[nodiscard]] virtual int get_required_ec_node_count () const = 0;
        [[nodiscard]] virtual int get_minimum_node_count () const = 0;
        [[nodiscard]] virtual int get_maximum_allowed_failed_nodes_count () const = 0;
        virtual void add_ec_node (uint128_t offset, std::shared_ptr <client> node) = 0;
        [[nodiscard]] virtual const std::vector <std::shared_ptr <client>>& get_ec_nodes () const = 0;
        [[nodiscard]] virtual const std::map <uint128_t, std::shared_ptr <client>>& get_ec_node_offset_map () const = 0;
        [[nodiscard]] virtual std::shared_ptr <client> get_ec_node (uint128_t offset) const = 0;
        [[nodiscard]] virtual std::vector <unique_buffer <char>> compute_ec (const std::string_view& data, int data_nodes_count) const = 0;
        [[nodiscard]] virtual std::vector <unique_buffer <char>> recover (const std::map <int, unique_buffer<char>>& data_pieces, int fail_count) const = 0;
        virtual ~ec () = default;
    };

} // namespace uh::cluster

#endif //CORE_EC_H
