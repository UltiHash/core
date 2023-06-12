//
// Created by masi on 6/3/23.
//

#ifndef CORE_UNLOCKED_REDBLACK_TREE_H
#define CORE_UNLOCKED_REDBLACK_TREE_H

#include <atomic>
#include "fragment_set_interface.h"
#include "storage/backends/smart_backend/smart_config.h"
#include "storage/backends/smart_backend/storage_types/fixed_managed_storage.h"
#include "storage/backends/smart_backend/storage_types/growing_plain_storage.h"

namespace uh::dbn::storage::smart::sets {


class unlocked_redblack_tree: public fragment_set_interface{

public:

    unlocked_redblack_tree (set_config set_conf, fixed_managed_storage& data_store);

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

    void balance (node& z);

    node directed_balance (node& z, direction_t d);

    [[nodiscard]] inline node get_node (uint64_t offset) const noexcept;

    inline node add_node () noexcept;

    inline void set_root (node& x) noexcept;

    void rotate (node& x, direction_t d);

    static inline uint64_t& get_child (const node& x, direction_t d) noexcept;

    static inline uint64_t& get_other_child (const node& x, direction_t d) noexcept;

    [[nodiscard]] inline int comp (const std::string_view& new_fragment, const fragment& f)const ;

    constexpr static uint64_t NILL_OFFSET = 2 * sizeof (uint64_t);

    const set_config m_set_conf;
    fixed_managed_storage& m_data_store;
    growing_plain_storage m_index_store;
    node m_nil {};
    std::atomic <uint64_t*> m_root;
    std::atomic_ref <uint64_t> m_end;
};

} // end namespace uh::dbn::storage::smart::sets

#endif //CORE_UNLOCKED_REDBLACK_TREE_H
