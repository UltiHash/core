//
// Created by masi on 10/17/23.
//

#ifndef CORE_EC_XOR_H
#define CORE_EC_XOR_H

#include "ec.h"

namespace uh::cluster {
struct ec_xor: public ec {

    [[nodiscard]] int get_acquired_ec_node_count () const override {
        return static_cast <int> (m_nodes.size());
    }

    [[nodiscard]] int get_required_ec_node_count () const override {
        return 1;
    }

    [[nodiscard]] int get_minimum_node_count () const override {
        return 3;
    }

    [[nodiscard]] int get_maximum_allowed_failed_nodes_count () const override {
        return 1;
    }

    void add_ec_node (uint128_t offset, std::shared_ptr <client> node) override {
        m_nodes.emplace_back(node);
        m_node_offset_map.emplace(offset, std::move (node));
    }

    [[nodiscard]] const std::vector <std::shared_ptr <client>>& get_ec_nodes () const override {
        return m_nodes;
    }

    [[nodiscard]] const std::map <uint128_t, std::shared_ptr <client>>& get_ec_node_offset_map () const override {
        return m_node_offset_map;
    }

    [[nodiscard]] std::shared_ptr <client> get_ec_node (uint128_t offset) const override {
        const auto pfd = m_node_offset_map.upper_bound (offset);
        if (pfd == m_node_offset_map.cbegin()) {
            throw std::out_of_range ("Could not find the corresponding EC node!");
        }
        return std::prev (pfd)->second;
    }

    [[nodiscard]] std::vector <ospan <char>> compute_ec (const std::string_view& data, int data_nodes_count) const override {
        const size_t part_size = data.size() / data_nodes_count;
        std::vector <ospan <char>> result;
        result.emplace_back(part_size);
        std::span <char> parity_view {result.back().data.get(), part_size};
        const auto long_size = (part_size - part_size % sizeof (unsigned long)) / sizeof (unsigned long);
        std::span <unsigned long> long_parity_view {reinterpret_cast <unsigned long*> (parity_view.data()),long_size};
        std::memset (result.back().data.get(), 0, part_size);
        for (int j = 0; j < data_nodes_count; j ++) {
            const auto data_part = data.substr(j * part_size, part_size);
            std::span <const unsigned long> long_data_part {reinterpret_cast <const unsigned long*> (data_part.data()), long_size};
            for (size_t k = 0; k < long_data_part.size(); ++k) {
                long_parity_view [k] ^= long_data_part[k];
            }
            for (size_t k = long_size * sizeof (unsigned long); k < part_size; k++) {
                parity_view [k] ^= data_part [k];
            }
        }
        return result;
    }

    [[nodiscard]] std::vector <ospan <char>> recover (const std::map <int, ospan<char>>& data_pieces, int fail_count) const override {
        const size_t part_size = data_pieces.cbegin()->second.size;
        std::vector <ospan <char>> result;
        result.emplace_back(part_size);
        std::span <char> parity_view {result.back().data.get(), part_size};
        const auto long_size = (part_size - part_size % sizeof (unsigned long)) / sizeof (unsigned long);
        std::span <unsigned long> long_parity_view {reinterpret_cast <unsigned long*> (parity_view.data()), long_size};
        std::memset (result.back().data.get(), 0, part_size);
        for (int j = 0; j < data_pieces.size(); j ++) {
            const auto data_part = std::string_view (data_pieces.at(j).data.get(), data_pieces.at(j).size);
            std::span <const unsigned long> long_data_part {reinterpret_cast <const unsigned long*> (data_part.data()), long_size};
            for (size_t k = 0; k < long_data_part.size(); ++k) {
                long_parity_view [k] ^= long_data_part[k];
            }
            for (size_t k = long_size * sizeof (unsigned long); k < part_size; k++) {
                parity_view [k] ^= data_part [k];
            }
        }
        return result;
    }

private:
    std::vector <std::shared_ptr <client>> m_nodes;
    std::map <uint128_t, std::shared_ptr <client>> m_node_offset_map;
};
} // namespace uh::cluster

#endif //CORE_EC_XOR_H
