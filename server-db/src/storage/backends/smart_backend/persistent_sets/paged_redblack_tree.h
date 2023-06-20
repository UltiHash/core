//
// Created by masi on 6/6/23.
//

#ifndef CORE_PAGED_REDBLACK_TREE_H
#define CORE_PAGED_REDBLACK_TREE_H

#include <variant>
#include <memory>
#include "set_interface.h"
#include "set_comparator_traits.h"
#include "index_mem_structures.h"
#include "storage/backends/smart_backend/smart_config.h"
#include "storage/backends/smart_backend/storage_types/storage_common.h"
#include "storage/backends/smart_backend/storage_types/growing_plain_storage.h"

namespace uh::dbn::storage::smart::sets {

template <typename Comparator = set_full_comparator>
class paged_redblack_tree: public set_interface {

public:

    paged_redblack_tree (set_config set_conf, managed_storage& data_store);

private:

    position_info do_push_back_pointer (const std::string_view& data, uint64_t data_offset, const position_info& pos) override;

    [[nodiscard]] position_info do_find (const std::string_view& data, const position_info& pos) const override;

    void do_sync (const position_info& pos) override;

    void do_remove (std::string_view& data, const position_info& pos) override;

    void balance (node& z);

    node directed_balance (node& z, direction_t d);

    [[nodiscard]] inline node get_node (uint64_t offset) const noexcept;

    inline node add_node (uint64_t parent) noexcept;

    void rotate (node& x, direction_t d);

    static inline uint64_t& get_child (const node& x, direction_t d) noexcept;

    static inline uint64_t& get_other_child (const node& x, direction_t d) noexcept;

    std::pair <uint64_t, block&> get_block (uint64_t node_offset) noexcept;

    const set_config m_set_conf;
    std::reference_wrapper <managed_storage> m_data_store;
    growing_plain_storage m_index_store;
    node m_nil {};
    first_block& m_first_block;
    Comparator m_comp;

    const size_t m_block_size;
};

} // end namespace uh::dbn::storage::smart::sets



#endif //CORE_PAGED_REDBLACK_TREE_H
