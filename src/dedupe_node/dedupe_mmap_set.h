//
// Created by masi on 6/6/23.
//

#ifndef CORE_DEDUPE_MMAP_SET_H
#define CORE_DEDUPE_MMAP_SET_H

#include <variant>
#include <memory>
#include <stack>
#include "dedupe_node/index_mem_structures.h"
#include <iostream>
#include "dedupe_node/growing_plain_storage.h"
#include "common/common.h"
#include <boost/interprocess/mapped_region.hpp>
#include <data_node/data_store.h>

namespace uh::cluster::dedupe {


    struct expert_index_type {
        std::uint64_t position {};
        int comp {};
    };

    struct expert_set_data {
        ospan <char> data;
        uint128_t data_offset;
        uint64_t index_offset;
        expert_set_data (ospan <char> os, uint128_t data_off, uint64_t index_off): data (std::move(os)), data_offset (data_off), index_offset (index_off) {}
        expert_set_data (expert_set_data&& sd) noexcept: data (std::move (sd.data)), data_offset(sd.data_offset), index_offset (sd.index_offset) {
            sd.data_offset = 0;
            sd.index_offset = 0;
        }
        expert_set_data (expert_set_data& sd) = delete;
    };

    struct expert_set_result {
        std::optional <expert_set_data> lower;
        std::optional <expert_set_data> match;
        std::optional <expert_set_data> upper;
        expert_index_type index;
        expert_set_result (std::optional <expert_set_data> lo, std::optional <expert_set_data> eq, std::optional <expert_set_data> up, expert_index_type idx = {}):
                lower (std::move (lo)),
                match (std::move (eq)),
                upper (std::move (up)),
                index (idx) {
        }
        expert_set_result (expert_set_result&& res) noexcept:
                lower (std::move (res.lower)),
                match (std::move (res.match)),
                upper (std::move (res.upper)),
                index (res.index) {
            res.lower = std::nullopt;
            res.match = std::nullopt;
            res.upper = std::nullopt;
        }
    };

    class dedupe_mmap_set {

        struct set_full_comparator {
            explicit set_full_comparator (global_data_view& storage): m_storage (storage) {}

            [[nodiscard]] inline std::pair <int, ospan <char>> operator () (const std::string_view& new_data, const mmap_node& expert_set_data) const {
                ospan <char> buf (expert_set_data.m_data.size);
                const auto f = boost::asio::co_spawn (*m_storage.get_executor(), m_storage.read (buf.data.get(), expert_set_data.m_data.pointer, expert_set_data.m_data.size), boost::asio::use_future);
                f.wait();
                return std::pair {new_data.compare({buf.data.get(), expert_set_data.m_data.size}), std::move (buf)};
            }

            [[nodiscard]] inline std::optional <std::pair <int, ospan <char>>> cached (const std::string_view& new_data, const mmap_node& set_data) const {
                if (const auto comp = new_data.substr(0,sizeof (set_data.data_prefix)).compare({reinterpret_cast <const char*>(&set_data.data_prefix), sizeof(set_data.data_prefix)}); comp != 0) {
                    return std::pair {comp, ospan<char>{}};
                }

                auto buf = m_storage.read_cache(set_data.m_data.pointer, set_data.m_data.size);
                if (buf.has_value())
                    return std::pair {new_data.compare(buf.value().get_str_view()), std::move (buf.value())};
                else {
                    return std::nullopt;
                }
            }
            global_data_view& m_storage;
        };

    public:

        dedupe_mmap_set (set_config set_conf, global_data_view& data_store) :
                m_set_conf (std::move (set_conf)),
                m_data_store (data_store),
                m_index_store (growing_plain_storage (m_set_conf.key_store_config)),
                m_first_block (reinterpret_cast <first_block*> (m_index_store.get_storage())),
                m_comp (data_store),
                m_block_size (boost::interprocess::mapped_region::get_page_size()) {

            if (m_set_conf.key_store_config.init_size < 2 * m_block_size) {
                throw std::logic_error ("set file size should be at least large enough for 2 pages");
            }

            if (m_first_block->root_offset == 0) {
                m_first_block->mix_block_offset = m_block_size;
                m_first_block->empty_block = 2 * sizeof (block);
                m_first_block->empty_hole_size = first_block::effective_node_space + block::effective_node_space;
                m_nil = add_node (0);
                m_nil.m_mnode->m_color = BLACK;
                m_first_block->nill_offset = m_nil.m_offset;
                m_first_block->root_offset = m_nil.m_offset;
            }
            else {
                m_nil = get_node (m_first_block->nill_offset);
            }
        }

        expert_index_type add_pointer (const std::string_view& data, uint128_t data_offset, const expert_index_type& pos) {

            if (pos.position == 0) [[unlikely]] {
                throw std::logic_error ("paged_redblack_tree relies on the given position. First call the find function.");
            }
            if (pos.comp == 0 and pos.position != m_first_block->nill_offset) {
                return pos;
            }

            node z = add_node (pos.position);
            z.m_mnode->m_parent = pos.position;
            const auto y = get_node(pos.position);
            if (pos.comp == 0) {
                m_first_block->root_offset = z.m_offset;
            }
            else if (pos.comp < 0) {
                y.m_mnode->m_left = z.m_offset;
            }
            else {
                y.m_mnode->m_right = z.m_offset;
            }
            z.m_mnode->m_left = m_first_block->nill_offset;
            z.m_mnode->m_right = m_first_block->nill_offset;
            z.m_mnode->m_color = RED;
            z.m_mnode->m_data = {data_offset, data.size()};
            z.m_mnode->data_prefix = *reinterpret_cast <const uint64_t*> (data.data());

            const auto offset = z.m_offset;

            balance (z);

            if (m_first_block->empty_block > m_index_store.get_size() - m_set_conf.set_minimum_free_space) {
                m_index_store.extend_mapping();
                m_first_block = reinterpret_cast <first_block*> (m_index_store.get_storage());
                m_nil = get_node (m_first_block->nill_offset);
            }

            return {.position = offset, .comp = pos.comp};
        }

        [[nodiscard]] expert_set_result find (const std::string_view& data) const {


            auto y = m_nil;

            auto x = get_node (m_first_block->root_offset);

            int comp_int = 0;
            node largest_lower = m_nil;
            node smallest_upper = m_nil;
            while (x.m_offset != m_first_block->nill_offset) {
                y = x;
                auto comp_res = m_comp.cached (data, *x.m_mnode);
                if (!comp_res.has_value ()) {
                    comp_res = m_comp (data, *x.m_mnode);
                }
                comp_int = comp_res.value().first;
                if (comp_int < 0) {
                    smallest_upper = x;
                    x = get_node (x.m_mnode->m_left);
                }
                else if (comp_int > 0) {
                    largest_lower = x;
                    x = get_node (x.m_mnode->m_right);
                }
                else {
                    return expert_set_result {std::nullopt, std::move (expert_set_data {std::move (comp_res.value().second), y.m_mnode->m_data.pointer, y.m_offset}), std::nullopt};
                }
            }

            auto low_data = fetch_cached_node_data(largest_lower);
            auto up_data = fetch_cached_node_data(smallest_upper);

            if (!low_data.has_value()) {
                low_data.emplace(fetch_node_data(largest_lower));
            }

            if (!up_data.has_value()) {
                up_data.emplace(fetch_node_data(smallest_upper));
            }

            return std::move (expert_set_result {expert_set_data {std::move (low_data.value()), largest_lower.m_mnode->m_data.pointer, largest_lower.m_offset},
                                             std::nullopt,
                                                 expert_set_data {std::move (up_data.value()), smallest_upper.m_mnode->m_data.pointer, smallest_upper.m_offset},
                                             {y.m_offset, comp_int}});
        }

        void sync (const expert_index_type& pos) {

            if (msync(align_ptr (m_index_store.get_storage() + pos.position), sizeof (mmap_node), MS_SYNC) != 0) {
                throw std::system_error (errno, std::system_category(), "persisted_redblack_tree_set could not sync the mmap data");
            }
        }


        void remove (std::string_view& data, const expert_index_type& pos) {
            throw std::runtime_error ("not implemented");
        }

        void update_pointer (uint64_t index_pointer, const fragment& data_span) noexcept {
            const auto n = get_node (reinterpret_cast<uint64_t>(m_index_store.get_storage() + index_pointer));
            n.m_mnode->m_data = data_span;
        }

    private:
        [[nodiscard]] inline std::optional<ospan <char>> fetch_cached_node_data (const node& n) const {
            if (n.m_offset == m_nil.m_offset) [[unlikely]] {
                return ospan <char> {};
            }
            return m_data_store.read_cache(n.m_mnode->m_data.pointer, n.m_mnode->m_data.size);
        }

        [[nodiscard]] inline ospan <char> fetch_node_data (const node& n) const {
            if (n.m_offset == m_nil.m_offset) [[unlikely]] {
                return ospan <char> {};
            }
            ospan <char> buffer (n.m_mnode->m_data.size);
            const auto f = boost::asio::co_spawn (*m_data_store.get_executor(), m_data_store.read(buffer.data.get(), n.m_mnode->m_data.pointer, n.m_mnode->m_data.size), boost::asio::use_future);
            f.wait();
            return std::move (buffer);
        }

        void balance (node& z) {
            auto parent = get_node (z.m_mnode->m_parent);
            while (parent.m_mnode->m_color == RED) {
                auto grand_parent = get_node(parent.m_mnode->m_parent);
                if (parent.m_offset == grand_parent.m_mnode->m_right) {
                    directed_balance (z, LEFT);
                }
                else {
                    directed_balance (z, RIGHT);
                }
                if (z.m_offset == m_first_block->root_offset) {
                    break;
                }
                parent = get_node (z.m_mnode->m_parent);
            }
            get_node(m_first_block->root_offset).m_mnode->m_color = BLACK;
        }

        void directed_balance (node& z, direction_t d) {
            auto parent = get_node (z.m_mnode->m_parent);
            auto grand_parent = get_node(parent.m_mnode->m_parent);

            auto y = get_node (get_child(grand_parent, d));
            if (y.m_mnode->m_color == RED) {
                y.m_mnode->m_color = BLACK;
                parent.m_mnode->m_color = BLACK;
                grand_parent.m_mnode->m_color = RED;
                z = grand_parent;
            }
            else {
                if (z.m_offset == get_child(parent, d)) {
                    z = parent;
                    rotate (z, static_cast <direction_t> (1 - d));
                    parent = get_node (z.m_mnode->m_parent);
                    grand_parent = get_node(parent.m_mnode->m_parent);
                }
                parent.m_mnode->m_color = BLACK;
                grand_parent.m_mnode->m_color = RED;
                rotate (grand_parent, d);
            }
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
                    m_first_block->empty_hole_size -= sizeof (mmap_node);
                }
                else if (m_first_block->empty_hole_size < m_set_conf.max_empty_hole_size) {
                    auto new_b = get_block(m_first_block->empty_block);
                    n = new_b.second.acquire_node();
                    n.m_offset += new_b.first;
                    m_first_block->empty_hole_size -= sizeof (mmap_node);
                    m_first_block->empty_block += sizeof (block);
                }
                else if (auto mix_b = get_block(m_first_block->mix_block_offset); !mix_b.second.full()) {
                    n = mix_b.second.acquire_node();
                    n.m_offset += mix_b.first;
                    m_first_block->empty_hole_size -= sizeof (mmap_node);
                }
                else {
                    auto new_mix_b = get_block(m_first_block->empty_block);
                    n = new_mix_b.second.acquire_node();
                    n.m_offset += new_mix_b.first;
                    m_first_block->empty_hole_size -= sizeof (mmap_node);
                    m_first_block->empty_block += sizeof (block);
                    m_first_block->mix_block_offset = new_mix_b.first;
                }
                return n;
            };

            if (parent < m_block_size) {
                return find_page_friendly_offset (std::pair <uint64_t, first_block&>{0, *m_first_block});
            }
            else {
                return find_page_friendly_offset (std::move (get_block(parent)));
            }
        }

        void rotate (node& x, direction_t d) {
            auto y = get_node(get_other_child(x, d));
            auto& yc = get_child(y, d);
            get_other_child(x, d) = yc;
            if (yc != m_first_block->nill_offset) {
                get_node (yc).m_mnode->m_parent = x.m_offset;
            }
            y.m_mnode->m_parent = x.m_mnode->m_parent;
            if (x.m_mnode->m_parent == m_first_block->nill_offset) {
                m_first_block->root_offset = y.m_offset;
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
        global_data_view& m_data_store;
        growing_plain_storage m_index_store;
        node m_nil {};
        first_block* m_first_block;
        set_full_comparator m_comp;

        const size_t m_block_size;
    };

} // end namespace uh::cluster::dedupe



#endif //CORE_DEDUPE_MMAP_SET_H