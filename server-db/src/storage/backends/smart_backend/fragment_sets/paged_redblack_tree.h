//
// Created by masi on 6/6/23.
//

#ifndef CORE_PAGED_REDBLACK_TREE_H
#define CORE_PAGED_REDBLACK_TREE_H

#include <variant>
#include "fragment_set_interface.h"
#include "index_mem_structures.h"
#include "storage/backends/smart_backend/smart_config.h"
#include "storage/backends/smart_backend/storage_types/fixed_managed_storage.h"
#include "storage/backends/smart_backend/storage_types/growing_plain_storage.h"

namespace uh::dbn::storage::smart::sets {

class paged_redblack_tree: public fragment_set_interface {

public:

    paged_redblack_tree (set_config set_conf, fixed_managed_storage& data_store);

private:

    position_info do_insert_index (const std::string_view& frag, uint64_t data_offset, const position_info& pos) override;

    [[nodiscard]] position_info do_find (const std::string_view& frag, const position_info& pos) const override;

    void do_sync (const position_info& pos) override;

    void do_remove (fragment& frag, const position_info& pos) override;

    enum direction_t: uint8_t {
        LEFT = 0,
        RIGHT = 1,
    };

    void balance (node& z);

    node directed_balance (node& z, direction_t d);

    [[nodiscard]] inline node get_node (uint64_t offset) const noexcept;

    inline node add_node (uint64_t parent) noexcept;

    void rotate (node& x, direction_t d);

    static inline uint64_t& get_child (const node& x, direction_t d) noexcept;

    static inline uint64_t& get_other_child (const node& x, direction_t d) noexcept;

    [[nodiscard]] inline int comp (const std::string_view& new_fragment, const fragment& f) const;

    std::pair <uint64_t, block&> get_block (uint64_t node_offset) noexcept;

    const set_config m_set_conf;
    fixed_managed_storage& m_data_store;
    growing_plain_storage m_index_store;
    node m_nil {};
    first_block& m_first_block;
    //blocked_memory m_blocked_mem;

    const size_t m_block_size;
};

} // end namespace uh::dbn::storage::smart::sets



#endif //CORE_PAGED_REDBLACK_TREE_H
