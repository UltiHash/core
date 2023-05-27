//
// Created by masi on 5/19/23.
//

#ifndef CORE_PERSISTED_REDBLACK_TREE_SET_H
#define CORE_PERSISTED_REDBLACK_TREE_SET_H

#include <span>
#include <cstring>
#include <atomic>
#include <optional>
#include <boost/thread.hpp>

#include "fixed_managed_storage.h"
#include "growing_plain_storage.h"

namespace uh::dbn::storage::smart {


struct fragment {
    fragment (uint64_t data_offset, size_t size):
        m_data_offset (data_offset),
        m_size (size) {}
    uint64_t m_data_offset;
    size_t m_size;
};

class persisted_redblack_tree_set {


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
    private:
        int comp {};
        friend persisted_redblack_tree_set;
    };

    persisted_redblack_tree_set(fixed_managed_storage& data_store, std::filesystem::path file);

    uint64_t insert_index (const std::string_view& frag, uint64_t data_offset, uint64_t hint = NILL_OFFSET);

    search_result find (const std::string_view& frag, uint64_t hint = NILL_OFFSET);

    void sync (uint64_t offset);

    uint64_t remove (fragment& frag, uint64_t hint);

private:

    std::pair <uint64_t, bool> resolve_hint (uint64_t hint, const std::string_view& frag);

    search_result unlocked_find (const std::string_view& frag, uint64_t hint);

    void balance (node& z);

    node directed_balance (node& z, direction_t d);

    std::pair<uint64_t, bool> lower_sister_inspect_hint (const node& n, const std::string_view& frag, int n_frag_comp);
    std::pair<uint64_t, bool> upper_sister_inspect_hint (const node& n, const std::string_view& frag, int n_frag_comp);

    inline node get_node (uint64_t offset) noexcept;

    inline node add_node () noexcept;

    inline void set_root (node& x) noexcept;

    void rotate (node& x, direction_t d);

    static inline uint64_t& get_child (const node& x, direction_t d) noexcept;

    static inline uint64_t& get_other_child (const node& x, direction_t d) noexcept;

    inline int comp (const std::string_view& new_fragment, const fragment& f);

    constexpr static size_t SET_INIT_FILE_SIZE = 1024ul;
    constexpr static size_t SET_FILE_EXTEND_LIMIT = 256ul;
    constexpr static uint64_t NILL_OFFSET = 2 * sizeof (uint64_t);

    fixed_managed_storage& m_data_store;
    std::filesystem::path m_file_path;
    node m_nil {};
    growing_plain_storage m_index_store;
    std::atomic <uint64_t*> m_root;
    std::atomic_ref <uint64_t> m_end;
    boost::shared_mutex m_mutex;
};

} // end namespace uh::dbn::storage::smart
#endif //CORE_PERSISTED_REDBLACK_TREE_SET_H
