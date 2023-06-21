//
// Created by masi on 6/6/23.
//

#ifndef CORE_PAGED_REDBLACK_TREE_H
#define CORE_PAGED_REDBLACK_TREE_H

#include <variant>
#include <memory>
#include <stack>
#include "set_interface.h"
#include "set_comparator_traits.h"
#include "index_mem_structures.h"
#include "storage/backends/smart_backend/smart_config.h"
#include "storage/backends/smart_backend/storage_types/storage_common.h"
#include "storage/backends/smart_backend/storage_types/growing_plain_storage.h"

#include <boost/interprocess/mapped_region.hpp>


namespace uh::dbn::storage::smart::sets {

template <typename Comparator = set_full_comparator>
class paged_redblack_tree: public set_interface {

public:

    paged_redblack_tree (set_config set_conf, managed_storage& data_store) :
            m_set_conf (std::move (set_conf)),
            m_data_store (data_store),
            m_index_store (growing_plain_storage (m_set_conf.key_store_config)),
            m_first_block (*(reinterpret_cast <first_block*> (m_index_store.get_storage()))),
            m_comp (data_store),
            m_block_size (boost::interprocess::mapped_region::get_page_size()) {

        if (m_set_conf.key_store_config.init_size < 2 * m_block_size) {
            throw std::logic_error ("set file size should be at list large enough for 2 pages");
        }

        if (m_first_block.root_offset == 0) {
            m_first_block.mix_block_offset = m_block_size;
            m_first_block.empty_block = 2;
            m_first_block.empty_hole_size = first_block::effective_node_space + block::effective_node_space;
            m_nil = add_node (0);
            m_nil.m_mnode->m_color = BLACK;
            m_first_block.nill_offset = m_nil.m_offset;
            m_first_block.root_offset = m_nil.m_offset;
        }
        else {
            m_nil = get_node (m_first_block.nill_offset);
        }
    }

private:

    index_type do_add_pointer (const std::string_view& data, uint64_t data_offset, const index_type& pos) override {

        if (pos.position == 0) {
            throw std::logic_error ("paged_redblack_tree relies on the given position. First call the find function.");
        }

        node z = add_node (pos.position);
        z.m_mnode->m_parent = pos.position;
        const auto y = get_node(pos.position);
        if (pos.comp == 0) {
            m_first_block.root_offset = z.m_offset;
        }
        else if (pos.comp < 0) {
            y.m_mnode->m_left = z.m_offset;
        }
        else {
            y.m_mnode->m_right = z.m_offset;
        }
        z.m_mnode->m_left = m_first_block.nill_offset;
        z.m_mnode->m_right = m_first_block.nill_offset;
        z.m_mnode->m_color = RED;
        z.m_mnode->m_data = {data_offset, data.size()};

        const auto offset = z.m_offset;

        balance (z);

        if (m_first_block.empty_block > m_index_store.get_size() - m_set_conf.set_minimum_free_space) {
            m_index_store.extend_mapping();
            m_first_block = *(reinterpret_cast <first_block*> (m_index_store.get_storage()));
            m_nil = get_node (m_first_block.nill_offset);
        }
        return {.position = offset, .comp = pos.comp};
    }


    [[nodiscard]] set_result do_find (const std::string_view& data, const index_type&) const override {

        auto y = m_nil;
        set_result res;
        auto x = get_node (m_first_block.root_offset);

        int comp_int = 0;
        node largest_lower = m_nil;
        node smallest_upper = m_nil;
        while (x.m_offset != m_first_block.nill_offset) {
            y = x;
            comp_int = m_comp (data, x.m_mnode->m_data);
            if (comp_int < 0) {
                largest_lower = x;
                x = get_node (x.m_mnode->m_left);
            }
            else if (comp_int > 0) {
                smallest_upper = x;
                x = get_node (x.m_mnode->m_right);
            }
            else {
                res.match = {y.m_mnode->m_data.m_data_offset, fetch_node_data(y)};
                break;
            }
        }

        if (!res.match) {
            res.lower = {largest_lower.m_mnode->m_data.m_data_offset, fetch_node_data (largest_lower)};
            res.upper = {smallest_upper.m_mnode->m_data.m_data_offset, fetch_node_data (smallest_upper)};
        }
        res.index = {y.m_offset, comp_int};
        return res;
    }

    [[nodiscard]] std::list<std::pair<uint64_t, std::string_view>> do_get_range (const std::span<char> &start_data, const std::span<char> &end_data) const override {

        // TODO if start data or end data are empty

        auto fstart = find ({start_data.data(), start_data.size()});

        uint64_t start_offset;
        if (fstart.match.has_value()) {
            start_offset = fstart.match->first;
        }
        else if (fstart.upper.has_value()) {
            start_offset = fstart.upper->first;
        }
        else {
            return {};
        }

        auto fend = find ({end_data.data(), end_data.size()});

        uint64_t end_offset;
        if (fend.match.has_value()) {
            end_offset = fend.match->first;
        }
        else if (fend.lower.has_value()) {
            end_offset = fend.lower->first;
        }
        else {
            return {};
        }

        std::list<std::pair<uint64_t, std::string_view>> result;

        auto n = get_node (start_offset);
        in_order_traverse (n.m_mnode->m_left, end_offset, result);

        // TODO traverse the parent until no right/left? child anymore

        return result;
    }

    void do_sync (const index_type& pos) override {

        if (msync(align_ptr (m_index_store.get_storage() + pos.position), sizeof (mmap_node), MS_SYNC) != 0) {
            throw std::system_error (errno, std::system_category(), "persisted_redblack_tree_set could not sync the mmap data");
        }
    }


    void do_remove (std::string_view& data, const index_type& pos) override {
        throw std::runtime_error ("not implemented");
    }

    void in_order_traverse (uint64_t start_offset, uint64_t end_offset, std::list<std::pair<uint64_t, std::string_view>> &result) const {

        const auto nil =  m_first_block.nill_offset;

        auto offset = start_offset;
        std::stack <uint64_t> nodes;

        do {
            if (offset != end_offset and offset != nil) {
                auto n = get_node(offset);
                offset = n.m_mnode->m_right;
                nodes.push(offset);
            }
            else {
                offset = nodes.top();
                nodes.pop();

                auto n = get_node(offset);
                result.emplace_back(offset, fetch_node_data(n));

                offset = n.m_mnode->m_left;
            }

        } while (!nodes.empty());

    }

    [[nodiscard]] inline std::string_view fetch_node_data (const node& n) const {
        char *ptr = static_cast <char *> (m_data_store.get().get_raw_ptr(n.m_mnode->m_data.m_data_offset));
        return {ptr, n.m_mnode->m_data.m_size};
    }

    void balance (node& z) {
        auto parent = get_node (z.m_mnode->m_parent);
        while (parent.m_mnode->m_color == RED) {
            auto grand_parent = get_node(parent.m_mnode->m_parent);
            if (parent.m_offset == grand_parent.m_mnode->m_left) {
                z = directed_balance (z, RIGHT);
            }
            else if (parent.m_offset == grand_parent.m_mnode->m_right) {
                z = directed_balance (z, LEFT);
            }
            else {
                break;
            }
            parent = get_node (z.m_mnode->m_parent);
        }
        get_node(m_first_block.root_offset).m_mnode->m_color = BLACK;
    }

    node directed_balance (node& z, direction_t d) {
        auto parent = get_node (z.m_mnode->m_parent);
        auto grand_parent = get_node(parent.m_mnode->m_parent);

        auto y = get_node (get_child(grand_parent, d));
        if (y.m_mnode->m_color == RED) {
            parent.m_mnode->m_color = BLACK;
            y.m_mnode->m_color = BLACK;
            grand_parent.m_mnode->m_color = RED;
            z = grand_parent;
        }
        else {
            if (z.m_offset == get_child(parent, d)) {
                z = parent;
                rotate (z, static_cast <direction_t> (1 - d));
            }
            parent.m_mnode->m_color = BLACK;
            grand_parent.m_mnode->m_color = RED;
            rotate (grand_parent, d);
        }
        return z;
    }

    [[nodiscard]] inline node get_node (uint64_t offset) const noexcept {
        return {offset, reinterpret_cast <mmap_node*> (m_index_store.get_storage() + offset)};
    }


    inline node add_node (uint64_t parent) noexcept {

        auto find_page_friendly_offset = [this] (auto b) {
            node n;
            if (!b.second.full()) {
                n = b.second.acquire_node();
                n.m_offset += b.first;
                m_first_block.empty_hole_size -= sizeof (mmap_node);
            }
            else if (m_first_block.empty_hole_size < m_set_conf.max_empty_hole_size) {
                auto new_b = get_block(m_first_block.empty_block);
                n = new_b.second.acquire_node();
                n.m_offset += new_b.first;
                m_first_block.empty_hole_size -= sizeof (mmap_node);
                m_first_block.empty_block += block::effective_node_space;
            }
            else if (auto mix_b = get_block(m_first_block.mix_block_offset); !mix_b.second.full()) {
                n = mix_b.second.acquire_node();
                n.m_offset += mix_b.first;
                m_first_block.empty_hole_size -= sizeof (mmap_node);
            }
            else {
                auto new_mix_b = get_block(m_first_block.empty_block);
                n = new_mix_b.second.acquire_node();
                n.m_offset += new_mix_b.first;
                m_first_block.empty_hole_size -= sizeof (mmap_node);
                m_first_block.empty_block += block::effective_node_space;
                m_first_block.mix_block_offset = new_mix_b.first;
            }
            return n;
        };

        if (parent < m_block_size) {
            return find_page_friendly_offset (std::pair <uint64_t, first_block&>{0, m_first_block});
        }
        else {
            return find_page_friendly_offset (std::move (get_block(parent)));
        }
    }

    void rotate (node& x, direction_t d) {
        auto y = get_node(get_other_child(x, d));
        auto& yc = get_child(y, d);
        get_other_child(x, d) = yc;
        if (yc != m_first_block.nill_offset) {
            get_node (yc).m_mnode->m_parent = x.m_offset;
        }
        y.m_mnode->m_parent = x.m_mnode->m_parent;
        if (x.m_mnode->m_parent == m_first_block.nill_offset) {
            m_first_block.root_offset = y.m_offset;
        }
        else if (auto& aunt = get_child(get_node (x.m_mnode->m_parent), d); x.m_offset == aunt) {
            aunt = y.m_offset;
        }
        else {
            get_other_child(get_node (x.m_mnode->m_parent), d) = y.m_offset;
        }
        yc = x.m_offset;
        x.m_mnode->m_parent = y.m_offset;
    }

    static inline uint64_t& get_child (const node& x, direction_t d) noexcept {
        if (d == LEFT) {
            return x.m_mnode->m_left;
        }
        else {
            return x.m_mnode->m_right;
        }
    }

    static inline uint64_t& get_other_child (const node& x, direction_t d) noexcept {
        if (d == RIGHT) {
            return x.m_mnode->m_left;
        }
        else {
            return x.m_mnode->m_right;
        }
    }

    std::pair <uint64_t, block&> get_block (uint64_t node_offset) noexcept {
        const auto offset = node_offset - node_offset % m_block_size;
        return {offset, *reinterpret_cast <block*> (m_index_store.get_storage() + offset)};
    }


    const set_config m_set_conf;
    std::reference_wrapper <managed_storage> m_data_store;
    growing_plain_storage m_index_store;
    node m_nil {};
    first_block& m_first_block;
    Comparator m_comp;

    const size_t m_block_size;
};

} // end namespace uh::dbn::storage::smart::sets



#endif //CORE_PAGED_REDBLACK_TREE_H
