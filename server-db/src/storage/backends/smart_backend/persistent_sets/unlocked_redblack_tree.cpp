//
// Created by masi on 6/3/23.
//

#include "unlocked_redblack_tree.h"
namespace uh::dbn::storage::smart::sets {

unlocked_redblack_tree::unlocked_redblack_tree(set_config set_conf, managed_storage& data_store):
        m_set_conf (std::move (set_conf)),
        m_data_store (data_store),
        m_index_store (growing_plain_storage (m_set_conf.key_store_config)),
        m_root (reinterpret_cast <uint64_t*> (m_index_store.get_storage())),
        m_end (*reinterpret_cast <uint64_t*> (m_index_store.get_storage() + sizeof(m_root))) {

    if (m_end == 0) {
        m_end = NILL_OFFSET;
        m_nil = add_node ();
        m_nil.m_mnode->m_color = BLACK;
        set_root (m_nil);
    }
    else {
        m_nil = get_node(NILL_OFFSET);
    }
}

index_type unlocked_redblack_tree::do_add_pointer (const std::string_view& data, uint64_t data_offset, const index_type& pos) {

    if (pos.position == 0) {
        throw std::logic_error ("unlocked_redblack_tree relies on the given position. First call the find function.");
    }

    node z = add_node ();
    z.m_mnode->m_parent = pos.position;

    const auto y = get_node(pos.position);
    if (pos.comp == 0) {
        set_root (z);
    }
    else if (pos.comp < 0) {
        y.m_mnode->m_left = z.m_offset;
    }
    else {
        y.m_mnode->m_right = z.m_offset;
    }
    z.m_mnode->m_left = NILL_OFFSET;
    z.m_mnode->m_right = NILL_OFFSET;
    z.m_mnode->m_color = RED;
    z.m_mnode->m_data = {data_offset, data.size()};

    const auto offset = z.m_offset;

    balance (z);

    if (m_end > m_index_store.get_size() - m_set_conf.set_minimum_free_space) {
        m_index_store.extend_mapping();
        m_nil = get_node(2 * sizeof (uint64_t));
        m_root = reinterpret_cast <uint64_t*> (m_index_store.get_storage());
        m_end = *reinterpret_cast <uint64_t*> (m_index_store.get_storage() + sizeof(m_root));
    }
    return {.position = offset, .comp = pos.comp};
}

set_result unlocked_redblack_tree::do_find (const std::string_view& frag, const index_type& pos) const {

    auto y = m_nil;
    set_result res;
    auto x = get_node (*m_root);

    int comp_int = 0;
    node largest_lower = m_nil;
    node smallest_upper = m_nil;
    while (x.m_offset != NILL_OFFSET) {
        y = x;
        comp_int = comp (frag, x.m_mnode->m_data);
        if (comp_int < 0) {
            smallest_upper = x;
            x = get_node (x.m_mnode->m_left);
        }
        else if (comp_int > 0) {
            largest_lower = x;
            x = get_node (x.m_mnode->m_right);
        }
        else {
            char* ptr = static_cast <char*> (m_data_store.get().get_raw_ptr(y.m_mnode->m_data.m_data_offset));
            res.match = {{ptr, y.m_mnode->m_data.m_size}, y.m_mnode->m_data.m_data_offset,y.m_offset};
            break;
        }
    }

    if (!res.match) {
        char *ptr_lower = static_cast <char *> (m_data_store.get().get_raw_ptr(largest_lower.m_mnode->m_data.m_data_offset));
        char *ptr_upper = static_cast <char *> (m_data_store.get().get_raw_ptr(smallest_upper.m_mnode->m_data.m_data_offset));
        res.lower = {{ptr_lower, largest_lower.m_mnode->m_data.m_size}, largest_lower.m_mnode->m_data.m_data_offset, largest_lower.m_offset};
        res.upper = {{ptr_upper, smallest_upper.m_mnode->m_data.m_size}, smallest_upper.m_mnode->m_data.m_data_offset, smallest_upper.m_offset};
    }
    res.index = {y.m_offset, comp_int};

    return res;
}

std::list<set_data>
unlocked_redblack_tree::do_get_range(const std::span<char> &start_data, const std::span<char> &end_data) const {
    throw std::runtime_error ("not implemented");
}

void unlocked_redblack_tree::do_sync (const index_type& pos) {
    if (msync(align_ptr (m_index_store.get_storage() + pos.position), sizeof (mmap_node), MS_SYNC) != 0) {
        throw std::system_error (errno, std::system_category(), "persisted_redblack_tree_set could not sync the mmap data");
    }
}

void unlocked_redblack_tree::do_remove(std::string_view& frag, const index_type &pos) {
    throw std::runtime_error ("not implemented");
}

void unlocked_redblack_tree::balance(node& z) {
    auto parent = get_node (z.m_mnode->m_parent);
    while (parent.m_mnode->m_color == RED) {
        auto grand_parent = get_node(parent.m_mnode->m_parent);
        if (parent.m_offset == grand_parent.m_mnode->m_left) {
            z = directed_balance (z, RIGHT);
        }
        else if (parent.m_offset == grand_parent.m_mnode->m_right) {
            z = directed_balance (z, LEFT);
        }
        if (z.m_offset == *m_root) {
            break;
        }
        parent = get_node (z.m_mnode->m_parent);
    }
    get_node(*m_root).m_mnode->m_color = BLACK;
}

node unlocked_redblack_tree::directed_balance(node& z, direction_t d) {
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

void unlocked_redblack_tree::rotate (node& x, direction_t d) {
    auto y = get_node(get_other_child(x, d));
    auto& yc = get_child(y, d);
    get_other_child(x, d) = yc;
    if (yc != NILL_OFFSET) {
        get_node (yc).m_mnode->m_parent = x.m_offset;
    }
    y.m_mnode->m_parent = x.m_mnode->m_parent;
    if (x.m_mnode->m_parent == NILL_OFFSET) {
        set_root (y);
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

node unlocked_redblack_tree::get_node(uint64_t offset) const noexcept {
    return {offset, reinterpret_cast <mmap_node*> (m_index_store.get_storage() + offset)};
}

node unlocked_redblack_tree::add_node() noexcept {
    node n;
    do {
        n.m_offset = m_end;
    }
    while (!m_end.compare_exchange_weak(n.m_offset, m_end + sizeof (mmap_node)));

    n.m_mnode = reinterpret_cast <mmap_node*> (m_index_store.get_storage() + n.m_offset);
    return n;
}

void unlocked_redblack_tree::set_root(node &x) noexcept {
    *m_root = x.m_offset;
}

int unlocked_redblack_tree::comp(const std::string_view &new_data, const offset_span &f) const {
    auto* p2 = m_data_store.get().get_raw_ptr(f.m_data_offset);
    const std::string_view strw2 {static_cast <char*> (p2), f.m_size};
    return new_data.compare(strw2);
}

uint64_t& unlocked_redblack_tree::get_child(const node &x, direction_t d) noexcept {
    if (d == LEFT) {
        return x.m_mnode->m_left;
    }
    else {
        return x.m_mnode->m_right;
    }
}

uint64_t& unlocked_redblack_tree::get_other_child(const node &x, direction_t d) noexcept {
    if (d == RIGHT) {
        return x.m_mnode->m_left;
    }
    else {
        return x.m_mnode->m_right;
    }
}

index_type null_index;

} // end namespace uh::dbn::storage::smart::sets
