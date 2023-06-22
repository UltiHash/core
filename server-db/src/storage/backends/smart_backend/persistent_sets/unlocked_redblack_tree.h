//
// Created by masi on 6/3/23.
//

#ifndef CORE_UNLOCKED_REDBLACK_TREE_H
#define CORE_UNLOCKED_REDBLACK_TREE_H

#include <atomic>
#include "set_interface.h"
#include "storage/backends/smart_backend/smart_config.h"
#include "storage/backends/smart_backend/storage_types/storage_common.h"
#include "storage/backends/smart_backend/storage_types/growing_plain_storage.h"
#include "index_mem_structures.h"

namespace uh::dbn::storage::smart::sets {


class unlocked_redblack_tree: public set_interface{

public:

    unlocked_redblack_tree (set_config set_conf, managed_storage& data_store);

private:

    index_type do_add_pointer (const std::string_view& data, uint64_t data_offset, const index_type& pos) override;

    [[nodiscard]] set_result do_find (const std::string_view& data, const index_type& pos) const override;

    [[nodiscard]] std::list<std::pair<uint64_t, std::string_view>> do_get_range (const std::span<char> &start_data, const std::span<char> &end_data) const override;
    
    void do_sync (const index_type& pos) override;

    void do_remove (std::string_view& data, const index_type& pos) override;

    void balance (node& z);

    node directed_balance (node& z, direction_t d);

    [[nodiscard]] inline node get_node (uint64_t offset) const noexcept;

    inline node add_node () noexcept;

    inline void set_root (node& x) noexcept;

    void rotate (node& x, direction_t d);

    static inline uint64_t& get_child (const node& x, direction_t d) noexcept;

    static inline uint64_t& get_other_child (const node& x, direction_t d) noexcept;

    [[nodiscard]] inline int comp (const std::string_view& new_data, const offset_span& f)const ;

    constexpr static uint64_t NILL_OFFSET = 2 * sizeof (uint64_t);

    const set_config m_set_conf;
    std::reference_wrapper <managed_storage> m_data_store;
    growing_plain_storage m_index_store;
    node m_nil {};
    std::atomic <uint64_t*> m_root;
    std::atomic_ref <uint64_t> m_end;
};

} // end namespace uh::dbn::storage::smart::sets

#endif //CORE_UNLOCKED_REDBLACK_TREE_H
