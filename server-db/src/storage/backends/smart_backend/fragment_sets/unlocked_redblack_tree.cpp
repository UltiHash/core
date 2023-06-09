//
// Created by masi on 6/3/23.
//

#include "unlocked_redblack_tree.h"
namespace uh::dbn::storage::smart::sets {

unlocked_redblack_tree::unlocked_redblack_tree(set_config set_conf, fixed_managed_storage& data_store):
        m_set_conf (std::move (set_conf)),
        m_data_store (data_store),
        m_index_store (growing_plain_storage (m_set_conf.fragment_set_path, m_set_conf.set_init_file_size)),
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

position_info unlocked_redblack_tree::do_insert_index (const std::string_view& frag, uint64_t data_offset, const position_info& pos) {

    if (pos.hint == 0) {
        throw std::logic_error ("unlocked_redblack_tree relies on the given position. First call the find function.");
    }

    node z = add_node ();
    z.m_mnode->m_parent = pos.hint;

    const auto y = get_node(pos.hint);
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
    z.m_mnode->m_frag = {data_offset, frag.size()};

    const auto offset = z.m_offset;

    balance (z);

    if (m_end > m_index_store.get_size() - m_set_conf.set_minimum_free_space) {
        m_index_store.extend_mapping();
        m_nil = get_node(2 * sizeof (uint64_t));
        m_root = reinterpret_cast <uint64_t*> (m_index_store.get_storage());
        m_end = *reinterpret_cast <uint64_t*> (m_index_store.get_storage() + sizeof(m_root));
    }
    return {.hint = offset, .comp = pos.comp};
}

position_info unlocked_redblack_tree::do_find (const std::string_view& frag, const position_info& pos) const {

    auto y = m_nil;
    position_info res;
    auto x = get_node (*m_root);

    int comp_int = 0;
    node largest_lower = m_nil;
    node smallest_upper = m_nil;
    while (x.m_offset != NILL_OFFSET) {
        y = x;
        comp_int = comp (frag, x.m_mnode->m_frag);
        if (comp_int < 0) {
            largest_lower = x;
            x = get_node (x.m_mnode->m_left);
        }
        else if (comp_int > 0) {
            smallest_upper = x;
            x = get_node (x.m_mnode->m_right);
        }
        else {
            char* ptr = static_cast <char*> (m_data_store.get_raw_ptr(y.m_mnode->m_frag.m_data_offset));
            res.match = {y.m_mnode->m_frag.m_data_offset, {ptr, y.m_mnode->m_frag.m_size}};
            break;
        }
    }

    if (!res.match) {
        char *ptr_lower = static_cast <char *> (m_data_store.get_raw_ptr(largest_lower.m_mnode->m_frag.m_data_offset));
        char *ptr_upper = static_cast <char *> (m_data_store.get_raw_ptr(smallest_upper.m_mnode->m_frag.m_data_offset));
        res.lower = {largest_lower.m_mnode->m_frag.m_data_offset, {ptr_lower, largest_lower.m_mnode->m_frag.m_size}};
        res.upper = {smallest_upper.m_mnode->m_frag.m_data_offset, {ptr_upper, smallest_upper.m_mnode->m_frag.m_size}};
    }
    res.hint = y.m_offset;
    res.comp = comp_int;
    return res;
}
void unlocked_redblack_tree::do_sync (const position_info& pos) {
    if (msync(align_ptr (m_index_store.get_storage() + pos.hint), sizeof (mmap_node), MS_SYNC) != 0) {
        throw std::system_error (errno, std::system_category(), "persisted_redblack_tree_set could not sync the mmap data");
    }
}

void unlocked_redblack_tree::do_remove(fragment &frag, const position_info &pos) {
    throw std::runtime_error ("not implemented");
}

void unlocked_redblack_tree::balance(unlocked_redblack_tree::node& z) {
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
    get_node(*m_root).m_mnode->m_color = BLACK;
}

unlocked_redblack_tree::node unlocked_redblack_tree::directed_balance(unlocked_redblack_tree::node& z,
                                                                      unlocked_redblack_tree::direction_t d) {
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

void unlocked_redblack_tree::rotate (unlocked_redblack_tree::node& x, unlocked_redblack_tree::direction_t d) {
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

unlocked_redblack_tree::node unlocked_redblack_tree::get_node(uint64_t offset) const noexcept {
    return {offset, reinterpret_cast <mmap_node*> (m_index_store.get_storage() + offset)};
}

unlocked_redblack_tree::node unlocked_redblack_tree::add_node() noexcept {
    node n;
    do {
        n.m_offset = m_end;
    }
    while (!m_end.compare_exchange_weak(n.m_offset, m_end + sizeof (mmap_node)));

    n.m_mnode = reinterpret_cast <mmap_node*> (m_index_store.get_storage() + n.m_offset);
    return n;
}

void unlocked_redblack_tree::set_root(unlocked_redblack_tree::node &x) noexcept {
    *m_root = x.m_offset;
}

int unlocked_redblack_tree::comp(const std::string_view &new_fragment, const fragment &f) const {
    auto* p2 = m_data_store.get_raw_ptr(f.m_data_offset);
    const std::string_view strw2 {static_cast <char*> (p2), f.m_size};
    return new_fragment.compare(strw2);
}

uint64_t& unlocked_redblack_tree::get_child(const unlocked_redblack_tree::node &x,
                                            unlocked_redblack_tree::direction_t d) noexcept {
    if (d == LEFT) {
        return x.m_mnode->m_left;
    }
    else {
        return x.m_mnode->m_right;
    }
}

uint64_t& unlocked_redblack_tree::get_other_child(const unlocked_redblack_tree::node &x,
                                                  unlocked_redblack_tree::direction_t d) noexcept {
    if (d == RIGHT) {
        return x.m_mnode->m_left;
    }
    else {
        return x.m_mnode->m_right;
    }
}
} // end namespace uh::dbn::storage::smart::sets
