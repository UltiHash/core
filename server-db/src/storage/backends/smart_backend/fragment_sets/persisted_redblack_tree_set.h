//
// Created by masi on 5/19/23.
//

#ifndef CORE_PERSISTED_REDBLACK_TREE_SET_H
#define CORE_PERSISTED_REDBLACK_TREE_SET_H

#include <span>
#include <cstring>
#include <atomic>
#include <optional>

#include "storage/backends/smart_backend/storage_types/fixed_managed_storage.h"
#include "storage/backends/smart_backend/storage_types/growing_plain_storage.h"
#include "storage/backends/smart_backend/smart_config.h"
#include "fragment_set_interface.h"

namespace uh::dbn::storage::smart::sets {

class persisted_redblack_tree_set: public fragment_set_interface {

public:

    persisted_redblack_tree_set (set_config set_conf, fixed_managed_storage& data_store);

private:

    position_info do_insert_index (const std::string_view& frag, uint64_t data_offset, const position_info& pos) override;

    [[nodiscard]] position_info do_find (const std::string_view& frag, const position_info& pos) const override;

    void do_sync (const position_info& pos) override;

    void do_remove (fragment& frag, const position_info& pos) override;

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

    std::pair <uint64_t, bool> resolve_hint (uint64_t hint, const std::string_view& frag) const;

    position_info unlocked_find (const std::string_view& frag, uint64_t hint) const;

    void balance (node& z);

    void print_set (std::ostream& out, uint64_t offset) const;

    node directed_balance (node& z, direction_t d);

    std::pair<uint64_t, bool> lower_sister_inspect_hint (const node& n, const std::string_view& frag, int n_frag_comp) const;
    std::pair<uint64_t, bool> upper_sister_inspect_hint (const node& n, const std::string_view& frag, int n_frag_comp) const;

    [[nodiscard]] inline node get_node (uint64_t offset)  const noexcept;

    inline node add_node () noexcept;

    inline void set_root (node& x) noexcept;

    void rotate (node& x, direction_t d);

    static inline uint64_t& get_child (const node& x, direction_t d) noexcept;

    static inline uint64_t& get_other_child (const node& x, direction_t d) noexcept;

    [[nodiscard]] inline int comp (const std::string_view& new_fragment, const fragment& f) const;

    constexpr static uint64_t NILL_OFFSET = 2 * sizeof (uint64_t);

    const set_config m_set_conf;
    fixed_managed_storage& m_data_store;
    growing_plain_storage m_index_store;
    node m_nil {};
    std::atomic <uint64_t*> m_root;
    std::atomic_ref <uint64_t> m_end;
    mutable std::shared_mutex m_mutex;
};

} // end namespace uh::dbn::storage::smart::sets
#endif //CORE_PERSISTED_REDBLACK_TREE_SET_H
