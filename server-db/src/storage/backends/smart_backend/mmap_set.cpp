//
// Created by masi on 5/22/23.
//
#include "mmap_set.h"

namespace uh::dbn::storage::smart {

mmap_set::mmap_set(mmap_storage &data_store, std::filesystem::path file) :
        m_data_store (data_store),
        m_file_path (std::move (file)) {

    const auto existing_index = std::filesystem::exists(m_file_path.c_str());
    const auto fd = open(m_file_path.c_str(), O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if (!existing_index) {
        ftruncate(fd, m_file_size);
    }
    else {
        m_file_size = lseek (fd, SEEK_END, 0);
    }

    m_index_store = static_cast <char*> (mmap(nullptr, m_file_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
    close(fd);

    m_root = reinterpret_cast <uint64_t*> (m_index_store);
    m_end = reinterpret_cast <uint64_t*> (m_index_store + sizeof(m_root));

    if (!existing_index) {
        *m_end = NILL_OFFSET;
        m_nil = add_node ();
        m_nil.m_node->color = BLACK;
        set_root (m_nil);
    }
    else {
        m_nil = get_node(NILL_OFFSET);
    }
}

uint64_t mmap_set::insert_data(const std::string_view& frag) {
    auto alloc = m_data_store.allocate(frag.size());
    std::memcpy(alloc.m_addr, frag.data(), frag.size());
    return alloc.m_offset;
}

uint64_t mmap_set::insert_index(const std::string_view &frag, uint64_t data_offset, uint64_t hint) {

    const auto f = find (frag, hint);
    if (f.match) {
        return f.hint;
    }

    const auto y = get_node(f.hint);
    node z = add_node ();
    z.m_node->parent = f.hint;

    if (f.comp == 0) {
        set_root (z);
    }
    else if (f.comp < 0) {
        y.m_node->left = z.offset;
    }
    else {
        y.m_node->right = z.offset;
    }
    z.m_node->left = NILL_OFFSET;
    z.m_node->right = NILL_OFFSET;
    z.m_node->color = RED;
    z.m_node->frag = {data_offset, frag.size()};

    const auto offset = z.offset;

    balance (z);

    if (*m_end > m_file_size - SET_FILE_EXTEND_LIMIT) {
        extend_mapping ();
    }
    return offset;
}

mmap_set::search_result mmap_set::find(const std::string_view &frag, uint64_t hint) {
    auto y = m_nil;
    search_result res;

    const auto resolved = resolve_hint (hint, frag);
    auto x = get_node (resolved.first);

    if (resolved.second) {
        char* ptr = static_cast <char*> (m_data_store.get_raw_ptr(x.m_node->frag.data_offset));
        res.match = {y.m_node->frag.data_offset, {ptr, y.m_node->frag.size}};
        return res;
    }

    int comp_int = 0;
    node largest_lower = m_nil;
    node smallest_upper = m_nil;
    while (x.offset != NILL_OFFSET) {
        y = x;
        comp_int = comp (frag, x.m_node->frag);
        if (comp_int < 0) {
            smallest_upper = x;
            x = get_node (x.m_node->left);
        }
        else if (comp_int > 0) {
            largest_lower = x;
            x = get_node (x.m_node->right);
        }
        else {
            char* ptr = static_cast <char*> (m_data_store.get_raw_ptr(y.m_node->frag.data_offset));
            res.match = {y.m_node->frag.data_offset, {ptr, y.m_node->frag.size}};
            break;
        }
    }

    if (!res.match) {
        char *ptr_lower = static_cast <char *> (m_data_store.get_raw_ptr(largest_lower.m_node->frag.data_offset));
        char *ptr_upper = static_cast <char *> (m_data_store.get_raw_ptr(smallest_upper.m_node->frag.data_offset));
        res.lower = {largest_lower.m_node->frag.data_offset, {ptr_lower, largest_lower.m_node->frag.size}};
        res.upper = {smallest_upper.m_node->frag.data_offset, {ptr_upper, smallest_upper.m_node->frag.size}};
    }
    res.hint = y.offset;
    res.comp = comp_int;
    return res;
}

std::pair<uint64_t, bool> mmap_set::resolve_hint(uint64_t hint, const std::string_view &frag) {

    if (hint == NILL_OFFSET) {
        return {*m_root, false};
    }

    const auto n = get_node (hint);

    const auto comp_n = comp (frag, n.m_node->frag);
    if (comp_n == 0) {
        return {n.offset, true};
    }

    if (n.m_node->parent == NILL_OFFSET) {
        return {*m_root, false};
    }
    const auto p = get_node (n.m_node->parent);


    if (hint == p.m_node->left and p.m_node->right != NILL_OFFSET) {
        const auto lower_sister = get_node(p.m_node->right);
        const auto comp_ls = comp (frag, lower_sister.m_node->frag);

        if (comp_ls == 0) {
            return {lower_sister.offset, true};
        }

        if (comp_n < 0 and comp_ls > 0) {
            const auto comp_p = comp (frag, p.m_node->frag);

            if (comp_p == 0) {
                return {p.offset, true};
            }

            else if (comp_p > 0 and n.m_node->left != NILL_OFFSET) {
                return {n.m_node->left, false};
            }
            else if (comp_p < 0 and lower_sister.m_node->right != NILL_OFFSET){
                return {lower_sister.m_node->right, false};
            }
            else {
                return {p.offset, false};
            }
        }

    }
    else if (hint == p.m_node->right and p.m_node->left != NILL_OFFSET) {
        const auto upper_sister = get_node(p.m_node->left);
        const auto comp_us = comp (frag, upper_sister.m_node->frag);

        if (comp_us == 0) {
            return {upper_sister.offset, true};
        }

        if (comp_n > 0 and comp_us < 0) {
            const auto comp_p = comp (frag, p.m_node->frag);

            if (comp_p == 0) {
                return {p.offset, true};
            }
            else if (comp_p > 0 and upper_sister.m_node->right != NILL_OFFSET) {
                return {upper_sister.m_node->right, false};
            }
            else if (comp_p < 0 and n.m_node->left != NILL_OFFSET) {
                return {n.m_node->left, false};
            }
            else {
                return {p.offset, false};
            }
        }

    }

    if (p.m_node->parent == NILL_OFFSET) {
        return {*m_root, false};
    }

    const auto gp = get_node(p.m_node->parent);

    const auto comp_p = comp (frag, p.m_node->frag);
    if (comp_p == 0) {
        return {p.offset, true};
    }

    if (p.offset == gp.m_node->left and gp.m_node->right != NILL_OFFSET) {
        const auto lower_aunt = get_node(gp.m_node->right);
        const auto comp_la = comp (frag, lower_aunt.m_node->frag);

        if (comp_la == 0) {
            return {lower_aunt.offset, true};
        }

        if (comp_p < 0 and comp_la > 0) {
            const auto comp_gp = comp (frag, p.m_node->frag);

            if (comp_gp == 0) {
                return {gp.offset, true};
            }
            else if (comp_gp > 0 and p.m_node->left != NILL_OFFSET) {
                return {p.m_node->left, false};
            }
            else if (comp_gp < 0 and lower_aunt.m_node->right != NILL_OFFSET){
                return {lower_aunt.m_node->right, false};
            }
            else {
                return {gp.offset, false};
            }
        }
    }
    else if (p.offset == gp.m_node->right and gp.m_node->left != NILL_OFFSET) {
        const auto upper_aunt = get_node(gp.m_node->left);
        const auto comp_ua = comp (frag, upper_aunt.m_node->frag);

        if (comp_ua == 0) {
            return {upper_aunt.offset, true};
        }

        if (comp_p > 0 and comp_ua < 0) {
            const auto comp_gp = comp (frag, p.m_node->frag);

            if (comp_gp == 0) {
                return {p.offset, true};
            }
            else if (comp_gp > 0 and upper_aunt.m_node->right != NILL_OFFSET) {
                return {upper_aunt.m_node->right, false};
            }
            else if (comp_gp < 0 and p.m_node->left != NILL_OFFSET) {
                return {p.m_node->left, false};
            }
            else {
                return {p.offset, false};
            }
        }
    }

    return {*m_root, false};
}

void mmap_set::extend_mapping() {
    msync (m_index_store, *m_end, MS_SYNC);
    munmap (m_index_store, m_file_size);

    m_file_size *= 2;

    const auto fd = open(m_file_path.c_str(), O_RDWR);
    ftruncate(fd, m_file_size);

    m_index_store = static_cast <char*> (mmap(nullptr, m_file_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
    close(fd);

    m_nil = get_node(2 * sizeof (uint64_t));
    m_root = reinterpret_cast <uint64_t*> (m_index_store);
    m_end = reinterpret_cast <uint64_t*> (m_index_store + sizeof(m_root));

}

void mmap_set::balance(mmap_set::node &z) {
    auto parent = get_node (z.m_node->parent);
    while (parent.m_node->color == RED) {
        auto grand_parent = get_node(parent.m_node->parent);
        if (parent.offset == grand_parent.m_node->left) {
            auto y = get_node (grand_parent.m_node->right);
            if (y.m_node->color == RED) {
                parent.m_node->color = BLACK;
                y.m_node->color = BLACK;
                grand_parent.m_node->color = RED;
                z = grand_parent;
                parent = get_node (z.m_node->parent);
                grand_parent = get_node(parent.m_node->parent);
            }
            else {
                if (z.offset == parent.m_node->right) {
                    z = parent;
                    parent = get_node (z.m_node->parent);
                    grand_parent = get_node(parent.m_node->parent);
                    left_rotate (z);
                }
                parent.m_node->color = BLACK;
                grand_parent.m_node->color = RED;
                right_rotate (grand_parent);
            }
        }
        else {
            auto y = get_node (grand_parent.m_node->left);
            if (y.m_node->color == RED) {
                parent.m_node->color = BLACK;
                y.m_node->color = BLACK;
                grand_parent.m_node->color = RED;
                z = grand_parent;
                parent = get_node (z.m_node->parent);
                grand_parent = get_node(parent.m_node->parent);
            }
            else {
                if (z.offset == parent.m_node->left) {
                    z = parent;
                    parent = get_node (z.m_node->parent);
                    grand_parent = get_node(parent.m_node->parent);
                    right_rotate (z);
                }
                parent.m_node->color = BLACK;
                grand_parent.m_node->color = RED;
                left_rotate (grand_parent);
            }
        }
    }
    get_node(*m_root).m_node->color = BLACK;
}

void mmap_set::left_rotate(mmap_set::node &x) {
    auto y = get_node(x.m_node->right);
    x.m_node->right = y.m_node->left;
    if (y.m_node->left != NILL_OFFSET) {
        get_node (y.m_node->left).m_node->parent = x.offset;
    }
    y.m_node->parent = x.m_node->parent;
    if (x.m_node->parent == NILL_OFFSET) {
        set_root (y);
    }
    else if (x.offset == get_node (x.m_node->parent).m_node->left) {
        get_node(x.m_node->parent).m_node->left = y.offset;
    }
    else {
        get_node(x.m_node->parent).m_node->right = y.offset;
    }
    y.m_node->left = x.offset;
    x.m_node->parent = y.offset;
}

void mmap_set::right_rotate(mmap_set::node &x) {
    auto y = get_node(x.m_node->left);
    x.m_node->left = y.m_node->right;
    if (y.m_node->right != NILL_OFFSET) {
        get_node (y.m_node->right).m_node->parent = x.offset;
    }
    y.m_node->parent = x.m_node->parent;
    if (x.m_node->parent == NILL_OFFSET) {
        set_root (y);
    }
    else if (x.offset == get_node (x.m_node->parent).m_node->right) {
        get_node(x.m_node->parent).m_node->right = y.offset;
    }
    else {
        get_node(x.m_node->parent).m_node->left = y.offset;
    }
    y.m_node->right = x.offset;
    x.m_node->parent = y.offset;
}

mmap_set::node mmap_set::get_node(uint64_t offset) noexcept {
    return {offset, reinterpret_cast <mmap_node*> (m_index_store + offset)};
}

mmap_set::node mmap_set::add_node() noexcept {
    node n;
    n.offset = *m_end;
    *m_end += sizeof (mmap_node);
    n.m_node = reinterpret_cast <mmap_node*> (m_index_store + n.offset);
    return n;
}

void mmap_set::set_root(mmap_set::node &x) noexcept {
    *m_root = x.offset;
}

int mmap_set::comp(const std::string_view &new_fragment, const fragment &f) {
    auto* p2 = m_data_store.get_raw_ptr(f.data_offset);
    const std::string_view strw2 {static_cast <char*> (p2), f.size};
    return new_fragment.compare(strw2);
}

} // end namespace uh::dbn::storage::smart
