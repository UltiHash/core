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
    [[nodiscard]] std::shared_ptr <client> get_ec_node (uint128_t offset) const override {throw std::runtime_error ("Not supported operation!");};
    [[nodiscard]] std::vector <ospan <char>> compute_ec (const std::string_view& data, int data_nodes_count) const override {throw std::runtime_error ("Not supported operation!");};
    [[nodiscard]] std::vector <ospan <char>> recover (const std::map <int, ospan<char>>& data_pieces, int fail_count) const override {throw std::runtime_error ("Not supported operation!");};
private:
    std::vector <std::shared_ptr <client>> m_nodes;

};
} // namespace uh::cluster

#endif //CORE_EC_NON_H
