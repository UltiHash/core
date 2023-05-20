//
// Created by masi on 5/19/23.
//

#ifndef CORE_MMAP_SET_H
#define CORE_MMAP_SET_H

#include <span>
#include <cstring>
#include <atomic>

#include "mmap_storage.h"

namespace uh::dbn::storage::smart {


struct fragment {
    uint64_t data_offset;
    size_t size;
};

class mmap_set {

    enum color_t : uint8_t
    {
        RED = 0,
        BLACK = 1
    };

    struct mmap_node {
        fragment frag;
        uint64_t parent;
        uint64_t left;
        uint64_t right;
        color_t color;
    };
    struct node {
        uint64_t offset;
        mmap_node* m_node;
    };

    node m_nil;
    std::atomic <uint64_t*> m_root;
    std::atomic <uint64_t*> m_end;

public:
    mmap_set(const file_mmap_info& file, mmap_storage& data_store):
        m_data_store (data_store) {

        const auto existing_index = std::filesystem::exists(file.path);
        const auto fd = open(file.path.c_str(), O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
        ftruncate(fd, file.max_size);
        m_index_store = static_cast <char*> (mmap(file.address, file.max_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
        close(fd);

        m_root = reinterpret_cast <uint64_t*> (m_index_store);
        m_end = reinterpret_cast <uint64_t*> (m_index_store + sizeof(m_root));

        if (!existing_index) {
            *m_end = 2 * sizeof (uint64_t);
            m_nil = add_node ();
            m_nil.m_node->color = BLACK;
            set_root (m_nil);
        }
    }

    /**
     * frag contains an allocated pointer on m_data_store, the data is written on that location
     * hint is a pointer on m_index_store from the previously found location of the nearest index
     * @param data_ptr
     */
    uint64_t insert (const std::string_view& frag, uint64_t hint = 0) {

        auto y = m_nil;
        auto x = get_node (*m_root);
        int comp_int = 0;

        while (x.offset != m_nil.offset) {
            y = x;
            comp_int = comp (frag, x.m_node->frag);
            if (comp_int < 0) {
                x = get_node (x.m_node->left);
            }
            else if (comp_int > 0) {
                x = get_node (x.m_node->right);
            }
            else {
                return x.offset;
            }
        }

        node z = add_node ();
        z.m_node->parent = y.offset;
        if (y.offset == m_nil.offset) {
            // root
            set_root (z);
        }
        else if (comp_int < 0) {
            y.m_node->left = z.offset;
        }
        else if (comp_int > 0) {
            y.m_node->right = z.offset;
        }
        z.m_node->left = m_nil.offset;
        z.m_node->right = m_nil.offset;
        z.m_node->color = RED;
        auto alloc = m_data_store.allocate(frag.size());
        std::memcpy (alloc.m_addr, frag.data(), frag.size());
        z.m_node->frag = {alloc.m_offset, frag.size()};

        const auto offset = z.offset;

        balance (z);

        return offset;
    }

    // if exact match if found, hint, data
    std::tuple <int, uint64_t, std::string_view> find (const std::string_view &frag, uint64_t hint = 0) {
        auto y = m_nil;
        auto x = get_node (*m_root);
        int comp_int = 0;

        while (x.offset != m_nil.offset) {
            y = x;
            comp_int = comp (frag, x.m_node->frag);
            if (comp_int < 0) {
                x = get_node (x.m_node->left);
            }
            else if (comp_int > 0) {
                x = get_node (x.m_node->right);
            }
            else {
                break;
            }
        }

        char* ptr = static_cast <char*> (m_data_store.get_raw_ptr(y.m_node->frag.data_offset));
        return {comp_int, y.offset, {ptr, y.m_node->frag.size}};
    }

    uint64_t remove (fragment frag, uint64_t hint);

private:

    void balance (node& z) {
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

    void left_rotate (node& x) {
        auto y = get_node(x.m_node->right);
        x.m_node->right = y.m_node->left;
        if (y.m_node->left != m_nil.offset) {
            get_node (y.m_node->left).m_node->parent = x.offset;
        }
        y.m_node->parent = x.m_node->parent;
        if (x.m_node->parent == m_nil.offset) {
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

    void right_rotate (node& x) {
        auto y = get_node(x.m_node->left);
        x.m_node->left = y.m_node->right;
        if (y.m_node->right != m_nil.offset) {
            get_node (y.m_node->right).m_node->parent = x.offset;
        }
        y.m_node->parent = x.m_node->parent;
        if (x.m_node->parent == m_nil.offset) {
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

    inline node get_node (uint64_t offset) noexcept {
        return {offset, reinterpret_cast <mmap_node*> (m_index_store + offset)};
    }

    inline node add_node () noexcept {
        node n;
        n.offset = *m_end;
        *m_end += sizeof (mmap_node);
        n.m_node = reinterpret_cast <mmap_node*> (m_index_store + n.offset);
        return n;
    }

    inline void set_root (node &x) noexcept {
        *m_root = x.offset;
    }

    inline int comp (const std::string_view& new_fragment, const fragment& f) {
        auto* p2 = m_data_store.get_raw_ptr(f.data_offset);
        const std::string_view strw2 {static_cast <char*> (p2), f.size};
        return new_fragment.compare(strw2);
    }

    mmap_storage &m_data_store;
    char* m_index_store = nullptr;
};

} // end namespace uh::dbn::storage::smart
#endif //CORE_MMAP_SET_H
