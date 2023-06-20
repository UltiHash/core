//
// Created by masi on 6/6/23.
//

#include "paged_redblack_tree.h"
#include <boost/interprocess/mapped_region.hpp>

namespace uh::dbn::storage::smart::sets {

position_info null_position;

template <typename Comparator>
paged_redblack_tree <Comparator>::paged_redblack_tree(set_config set_conf, managed_storage& data_store):
        m_set_conf (std::move (set_conf)),
        m_data_store (data_store),
        m_index_store (growing_plain_storage (m_set_conf.fragment_set_path, m_set_conf.set_init_file_size)),
        m_first_block (*(reinterpret_cast <first_block*> (m_index_store.get_storage()))),
        m_comp (data_store),
        m_block_size (boost::interprocess::mapped_region::get_page_size()) {

    if (m_set_conf.set_init_file_size < 2 * m_block_size) {
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

template <typename Comparator>
position_info paged_redblack_tree <Comparator>::do_push_back_pointer (const std::string_view& data, uint64_t data_offset, const position_info& pos) {

    if (pos.hint == 0) {
        throw std::logic_error ("unlocked_redblack_tree relies on the given position. First call the find function.");
    }

    node z = add_node (pos.hint);
    z.m_mnode->m_parent = pos.hint;
    const auto y = get_node(pos.hint);
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
    return {.hint = offset, .comp = pos.comp};
}

template <typename Comparator>
position_info paged_redblack_tree <Comparator>::do_find (const std::string_view &data, const position_info&) const {

    auto y = m_nil;
    position_info res;
    auto x = get_node (m_first_block.root_offset);

    int comp_int = 0;
    node largest_lower = m_nil;
    node smallest_upper = m_nil;
    while (x.m_offset != m_first_block.nill_offset) {
        y = x;
        comp_int = (*m_comp) (data, x.m_mnode->m_data);
        if (comp_int < 0) {
            largest_lower = x;
            x = get_node (x.m_mnode->m_left);
        }
        else if (comp_int > 0) {
            smallest_upper = x;
            x = get_node (x.m_mnode->m_right);
        }
        else {
            char* ptr = static_cast <char*> (m_data_store.get().get_raw_ptr(y.m_mnode->m_data.m_data_offset));
            res.match = {y.m_mnode->m_data.m_data_offset, {ptr, y.m_mnode->m_data.m_size}};
            break;
        }
    }

    if (!res.match) {
        char *ptr_lower = static_cast <char *> (m_data_store.get().get_raw_ptr(largest_lower.m_mnode->m_data.m_data_offset));
        char *ptr_upper = static_cast <char *> (m_data_store.get().get_raw_ptr(smallest_upper.m_mnode->m_data.m_data_offset));
        res.lower = {largest_lower.m_mnode->m_data.m_data_offset, {ptr_lower, largest_lower.m_mnode->m_data.m_size}};
        res.upper = {smallest_upper.m_mnode->m_data.m_data_offset, {ptr_upper, smallest_upper.m_mnode->m_data.m_size}};
    }
    res.hint = y.m_offset;
    res.comp = comp_int;
    return res;
}

template <typename Comparator>
void paged_redblack_tree <Comparator>::do_sync(const position_info& pos) {

    if (msync(align_ptr (m_index_store.get_storage() + pos.hint), sizeof (mmap_node), MS_SYNC) != 0) {
        throw std::system_error (errno, std::system_category(), "persisted_redblack_tree_set could not sync the mmap data");
    }
}

template <typename Comparator>
void paged_redblack_tree <Comparator>::do_remove(std::string_view& data, const position_info &pos) {
    throw std::runtime_error ("not implemented");
}

template <typename Comparator>
void paged_redblack_tree <Comparator>::balance(node& z) {
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

template <typename Comparator>
node paged_redblack_tree <Comparator>::directed_balance(node& z, direction_t d) {
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

template <typename Comparator>
void paged_redblack_tree <Comparator>::rotate (node& x, direction_t d) {
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

template <typename Comparator>
node paged_redblack_tree <Comparator>::get_node(uint64_t offset) const noexcept {
    return {offset, reinterpret_cast <mmap_node*> (m_index_store.get_storage() + offset)};
}

template <typename Comparator>
node paged_redblack_tree <Comparator>::add_node(uint64_t parent) noexcept {

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

template <typename Comparator>
uint64_t& paged_redblack_tree <Comparator>::get_child(const node &x, direction_t d) noexcept {
    if (d == LEFT) {
        return x.m_mnode->m_left;
    }
    else {
        return x.m_mnode->m_right;
    }
}

template <typename Comparator>
uint64_t& paged_redblack_tree <Comparator>::get_other_child(const node &x, direction_t d) noexcept {
    if (d == RIGHT) {
        return x.m_mnode->m_left;
    }
    else {
        return x.m_mnode->m_right;
    }
}

template <typename Comparator>
std::pair <uint64_t, block&> paged_redblack_tree <Comparator>::get_block (uint64_t node_offset) noexcept {
    const auto offset = node_offset - node_offset % m_block_size;
    return {offset, *reinterpret_cast <block*> (m_index_store.get_storage() + offset)};
}

} // end namespace uh::dbn::storage::smart::sets
