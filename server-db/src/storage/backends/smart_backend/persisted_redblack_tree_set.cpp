//
// Created by masi on 5/22/23.
//
#include <iostream>
#include "persisted_redblack_tree_set.h"

namespace uh::dbn::storage::smart {

persisted_redblack_tree_set::persisted_redblack_tree_set(set_config set_conf, fixed_managed_storage& data_store):
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

uint64_t persisted_redblack_tree_set::insert_index(const std::string_view& frag, uint64_t data_offset, uint64_t hint) {

    std::lock_guard lock (m_mutex);

    const auto f = unlocked_find (frag, hint);
    if (f.match) {
        return f.hint;
    }

    node z = add_node ();
    z.m_mnode->m_parent = f.hint;

    const auto y = get_node(f.hint);
    if (f.comp == 0) {
        set_root (z);
    }
    else if (f.comp < 0) {
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
    return offset;
}

persisted_redblack_tree_set::search_result persisted_redblack_tree_set::find(const std::string_view& frag, uint64_t hint) {

    std::shared_lock lock (m_mutex);
    return unlocked_find (frag, hint);
}

void persisted_redblack_tree_set::sync(uint64_t offset) {

    if (msync(align_ptr (m_index_store.get_storage() + offset), sizeof (mmap_node), MS_SYNC) != 0) {
        throw std::system_error (errno, std::system_category(), "persisted_redblack_tree_set could not sync the mmap data");
    }
}

std::pair<uint64_t, bool> persisted_redblack_tree_set::resolve_hint(uint64_t hint, const std::string_view& frag) {

    if (hint == NILL_OFFSET) {
        return {*m_root, false};
    }

    const auto n = get_node (hint);

    const auto comp_n = comp (frag, n.m_mnode->m_frag);
    if (comp_n == 0) {
        return {n.m_offset, true};
    }

    if (n.m_mnode->m_parent == NILL_OFFSET) {
        return {*m_root, false};
    }
    const auto p = get_node (n.m_mnode->m_parent);
    
    if (hint == p.m_mnode->m_left and p.m_mnode->m_right != NILL_OFFSET) {
        const auto res = lower_sister_inspect_hint(n, frag, comp_n);
        if (res.first != NILL_OFFSET) {
            return res;
        }
    }
    else if (hint == p.m_mnode->m_right and p.m_mnode->m_left != NILL_OFFSET) {
        const auto res = upper_sister_inspect_hint (n, frag, comp_n);
        if (res.first != NILL_OFFSET) {
            return res;
        }
    }

    if (p.m_mnode->m_parent == NILL_OFFSET) {
        return {*m_root, false};
    }

    const auto gp = get_node(p.m_mnode->m_parent);

    const auto comp_p = comp (frag, p.m_mnode->m_frag);
    if (comp_p == 0) {
        return {p.m_offset, true};
    }

    if (p.m_offset == gp.m_mnode->m_left and gp.m_mnode->m_right != NILL_OFFSET) {
        const auto res = lower_sister_inspect_hint(p, frag, comp_p);
        if (res.first != NILL_OFFSET) {
            return res;
        }
    }
    else if (p.m_offset == gp.m_mnode->m_right and gp.m_mnode->m_left != NILL_OFFSET) {
        const auto res = upper_sister_inspect_hint (p, frag, comp_p);
        if (res.first != NILL_OFFSET) {
            return res;
        }
    }

    return {*m_root, false};
}

std::pair<uint64_t, bool> persisted_redblack_tree_set::lower_sister_inspect_hint(const persisted_redblack_tree_set::node &n,
                                                                           const std::string_view& frag,
                                                                           int n_frag_comp) {
    const auto p = get_node (n.m_mnode->m_parent);

    const auto lower_sister = get_node(p.m_mnode->m_right);
    const auto comp_ls = comp (frag, lower_sister.m_mnode->m_frag);

    if (comp_ls == 0) {
        return {lower_sister.m_offset, true};
    }

    if (n_frag_comp > 0 and comp_ls < 0) {
        const auto comp_p = comp (frag, p.m_mnode->m_frag);

        if (comp_p == 0) {
            return {p.m_offset, true};
        }

        else if (comp_p < 0 and n.m_mnode->m_left != NILL_OFFSET) {
            return {n.m_mnode->m_right, false};
        }
        else if (comp_p > 0 and lower_sister.m_mnode->m_right != NILL_OFFSET){
            return {lower_sister.m_mnode->m_left, false};
        }
        else {
            return {p.m_offset, false};
        }
    }
    return {NILL_OFFSET, false};
}


std::pair<uint64_t, bool>
persisted_redblack_tree_set::upper_sister_inspect_hint(const persisted_redblack_tree_set::node &n,
                                                       const std::string_view &frag, int n_frag_comp) {
    const auto p = get_node (n.m_mnode->m_parent);

    const auto upper_sister = get_node(p.m_mnode->m_left);
    const auto comp_us = comp (frag, upper_sister.m_mnode->m_frag);

    if (comp_us == 0) {
        return {upper_sister.m_offset, true};
    }

    if (n_frag_comp < 0 and comp_us > 0) {
        const auto comp_p = comp (frag, p.m_mnode->m_frag);

        if (comp_p == 0) {
            return {p.m_offset, true};
        }
        else if (comp_p < 0 and upper_sister.m_mnode->m_right != NILL_OFFSET) {
            return {upper_sister.m_mnode->m_right, false};
        }
        else if (comp_p > 0 and n.m_mnode->m_left != NILL_OFFSET) {
            return {n.m_mnode->m_left, false};
        }
        else {
            return {p.m_offset, false};
        }
    }
    return {NILL_OFFSET, false};

}

persisted_redblack_tree_set::search_result persisted_redblack_tree_set::unlocked_find (const std::string_view &frag, uint64_t hint) {

    auto y = m_nil;
    search_result res;

    const auto resolved = resolve_hint (hint, frag);
    auto x = get_node (resolved.first);

    if (resolved.second) {
        char* ptr = static_cast <char*> (m_data_store.get_raw_ptr(x.m_mnode->m_frag.m_data_offset));
        res.match = {y.m_mnode->m_frag.m_data_offset, {ptr, y.m_mnode->m_frag.m_size}};
        return res;
    }

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

void persisted_redblack_tree_set::balance(persisted_redblack_tree_set::node& z) {
    auto parent = get_node (z.m_mnode->m_parent);
    while (parent.m_mnode->m_color == RED) {
        auto grand_parent = get_node(parent.m_mnode->m_parent);
        if (parent.m_offset == grand_parent.m_mnode->m_left) {
            z = directed_balance (z, RIGHT);
        }
        else {
            z = directed_balance (z, LEFT);
        }
        parent = get_node (z.m_mnode->m_parent);
    }
    get_node(*m_root).m_mnode->m_color = BLACK;
}

persisted_redblack_tree_set::node persisted_redblack_tree_set::directed_balance(persisted_redblack_tree_set::node& z,
                                                   persisted_redblack_tree_set::direction_t d) {
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

void persisted_redblack_tree_set::rotate (persisted_redblack_tree_set::node& x, persisted_redblack_tree_set::direction_t d) {
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

persisted_redblack_tree_set::node persisted_redblack_tree_set::get_node(uint64_t offset) noexcept {
    return {offset, reinterpret_cast <mmap_node*> (m_index_store.get_storage() + offset)};
}

persisted_redblack_tree_set::node persisted_redblack_tree_set::add_node() noexcept {
    node n;
    do {
        n.m_offset = m_end;
    }
    while (!m_end.compare_exchange_weak(n.m_offset, m_end + sizeof (mmap_node)));

    n.m_mnode = reinterpret_cast <mmap_node*> (m_index_store.get_storage() + n.m_offset);
    return n;
}

void persisted_redblack_tree_set::set_root(persisted_redblack_tree_set::node &x) noexcept {
    *m_root = x.m_offset;
}

int persisted_redblack_tree_set::comp(const std::string_view &new_fragment, const fragment &f) {
    auto* p2 = m_data_store.get_raw_ptr(f.m_data_offset);
    const std::string_view strw2 {static_cast <char*> (p2), f.m_size};
    return new_fragment.compare(strw2);
}

uint64_t& persisted_redblack_tree_set::get_child(const persisted_redblack_tree_set::node &x,
                                               persisted_redblack_tree_set::direction_t d) noexcept {
    if (d == LEFT) {
        return x.m_mnode->m_left;
    }
    else {
        return x.m_mnode->m_right;
    }
}

uint64_t& persisted_redblack_tree_set::get_other_child(const persisted_redblack_tree_set::node &x,
                                                   persisted_redblack_tree_set::direction_t d) noexcept {
    if (d == RIGHT) {
        return x.m_mnode->m_left;
    }
    else {
        return x.m_mnode->m_right;
    }
}

void persisted_redblack_tree_set::print_set(std::ostream& out, uint64_t offset) {
    node n = get_node(offset);
    auto* p = m_data_store.get_raw_ptr(n.m_mnode->m_frag.m_data_offset);
    const std::string_view str {static_cast <char*> (p), n.m_mnode->m_frag.m_size};
    std::string_view strl;
    if (n.m_mnode->m_left != NILL_OFFSET) {
        node l = get_node(n.m_mnode->m_left);
        auto *pl = m_data_store.get_raw_ptr(l.m_mnode->m_frag.m_data_offset);
        strl = std::string_view {static_cast <char *> (pl), l.m_mnode->m_frag.m_size};
    }
    std::string_view strr;
    if (n.m_mnode->m_right != NILL_OFFSET) {
        node r = get_node(n.m_mnode->m_right);
        auto *pr = m_data_store.get_raw_ptr(r.m_mnode->m_frag.m_data_offset);
        strr = std::string_view {static_cast <char *> (pr), r.m_mnode->m_frag.m_size};
    }
    out << str << "\n\t" << "left: " << strl << "\n\t" << "right: " << strr << "\n";

    if (n.m_mnode->m_left != NILL_OFFSET) {
        print_set(out, n.m_mnode->m_left);

    }
    if (n.m_mnode->m_right != NILL_OFFSET) {
        print_set(out, n.m_mnode->m_right);

    }
}

} // end namespace uh::dbn::storage::smart
