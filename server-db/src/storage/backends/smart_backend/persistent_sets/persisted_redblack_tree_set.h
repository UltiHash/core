//
// Created by masi on 5/19/23.
//

#ifndef CORE_PERSISTED_REDBLACK_TREE_SET_H
#define CORE_PERSISTED_REDBLACK_TREE_SET_H

#include <span>
#include <cstring>
#include <atomic>
#include <optional>

#include "storage/backends/smart_backend/storage_types/storage_common.h"
#include "storage/backends/smart_backend/storage_types/growing_plain_storage.h"
#include "storage/backends/smart_backend/smart_config.h"
#include "set_interface.h"
#include "index_mem_structures.h"

namespace uh::dbn::storage::smart::sets {

class persisted_redblack_tree_set: public set_interface {

public:

    persisted_redblack_tree_set (set_config set_conf, managed_storage& data_store);

private:

    index_type do_add_pointer (const std::string_view& frag, uint64_t data_offset, const index_type& pos) override;

    [[nodiscard]] set_result do_find (const std::string_view& frag, const index_type& pos) const override;

    void do_sync (const index_type& pos) override;

    void do_remove (std::string_view &frag, const index_type& pos) override;

    std::pair <uint64_t, bool> resolve_hint (uint64_t hint, const std::string_view& frag) const;

    set_result unlocked_find (const std::string_view& frag, index_type hint) const;

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

    [[nodiscard]] inline int comp (const std::string_view& new_data, const offset_span& f) const;

    constexpr static uint64_t NILL_OFFSET = 2 * sizeof (uint64_t);

    const set_config m_set_conf;
    std::reference_wrapper <managed_storage> m_data_store;
    growing_plain_storage m_index_store;
    node m_nil {};
    std::atomic <uint64_t*> m_root;
    std::atomic_ref <uint64_t> m_end;
    mutable std::shared_mutex m_mutex;
};

} // end namespace uh::dbn::storage::smart::sets
#endif //CORE_PERSISTED_REDBLACK_TREE_SET_H
