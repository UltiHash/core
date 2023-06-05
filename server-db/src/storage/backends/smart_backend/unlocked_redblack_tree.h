//
// Created by masi on 6/3/23.
//

#ifndef CORE_UNLOCKED_REDBLACK_TREE_H
#define CORE_UNLOCKED_REDBLACK_TREE_H

#include "persisted_redblack_tree_set.h"

namespace uh::dbn::storage::smart {


class unlocked_redblack_tree {

    enum color_t : uint8_t
    {
        RED = 0,
        BLACK = 1
    };

    enum direction_t: uint8_t {
        LEFT = 0,
        RIGHT = 1,
    };

    struct mmap_node {
        fragment m_frag;
        uint64_t m_parent;
        uint64_t m_left;
        uint64_t m_right;
        color_t m_color;
    };
    struct node {
        uint64_t m_offset;
        mmap_node* m_mnode;
    };

public:
    struct search_result {
        std::optional <std::pair <uint64_t, std::string_view>> lower;
        std::optional <std::pair <uint64_t, std::string_view>> match;
        std::optional <std::pair <uint64_t, std::string_view>> upper;
        uint64_t hint {};
        int comp {};
    };



    unlocked_redblack_tree (set_config set_conf, fixed_managed_storage& data_store);

    uint64_t insert_index (const std::string_view& frag, uint64_t data_offset, const search_result& pos);

    search_result find (const std::string_view& frag);

    void sync (uint64_t offset);

    uint64_t remove (fragment& frag, uint64_t hint);

private:

    void balance (node& z);

    node directed_balance (node& z, direction_t d);

    inline node get_node (uint64_t offset) noexcept;

    inline node add_node () noexcept;

    inline void set_root (node& x) noexcept;

    void rotate (node& x, direction_t d);

    static inline uint64_t& get_child (const node& x, direction_t d) noexcept;

    static inline uint64_t& get_other_child (const node& x, direction_t d) noexcept;

    inline int comp (const std::string_view& new_fragment, const fragment& f);

    constexpr static uint64_t NILL_OFFSET = 2 * sizeof (uint64_t);

    const set_config m_set_conf;
    fixed_managed_storage& m_data_store;
    growing_plain_storage m_index_store;
    node m_nil {};
    std::atomic <uint64_t*> m_root;
    std::atomic_ref <uint64_t> m_end;
};

} // end namespace uh::dbn::storage::smart

#endif //CORE_UNLOCKED_REDBLACK_TREE_H
