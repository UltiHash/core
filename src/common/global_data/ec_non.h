//
// Created by masi on 10/17/23.
//

#ifndef CORE_EC_NON_H
#define CORE_EC_NON_H

#include "ec.h"

namespace uh::cluster {
struct ec_non: public ec {
    [[nodiscard]] int get_acquired_ec_node_count () const override {return 0;}
    [[nodiscard]] int get_required_ec_node_count () const override {return 0;}
    [[nodiscard]] int get_minimum_node_count () const override {return 1;}
    [[nodiscard]] int get_maximum_allowed_failed_nodes_count () const override {return 0;}
    void add_ec_node (uint128_t offset, std::shared_ptr <client> node) override {throw std::runtime_error ("Not supported operation!");}
    [[nodiscard]] const std::vector <std::shared_ptr <client>>& get_ec_nodes () const override {return m_nodes;};
    [[nodiscard]] const std::map <uint128_t, std::shared_ptr <client>>& get_ec_node_offset_map () const override {return m_node_offset_map;}
    [[nodiscard]] std::shared_ptr <client> get_ec_node (uint128_t offset) const override {throw std::runtime_error ("Not supported operation!");};
    [[nodiscard]] std::vector <unique_buffer <char>> compute_ec (const std::string_view& data, int data_nodes_count) const override {return {};};
    [[nodiscard]] std::vector <unique_buffer <char>> recover (const std::map <int, unique_buffer<char>>& data_pieces, int fail_count) const override {throw std::runtime_error ("Not supported operation!");};
private:
    std::vector <std::shared_ptr <client>> m_nodes;
    std::map <uint128_t, std::shared_ptr <client>> m_node_offset_map;

};
} // namespace uh::cluster

#endif //CORE_EC_NON_H
