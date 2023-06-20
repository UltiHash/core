//
// Created by masi on 5/22/23.
//

#ifndef CORE_SMART_CORE_H
#define CORE_SMART_CORE_H

#include "storage/backends/smart_backend/persistent_sets/persisted_redblack_tree_set.h"
#include "storage/backends/smart_backend/storage_types/fixed_managed_storage.h"
#include "storage/backends/smart_backend/key_stores/persisted_robinhood_hashmap.h"
#include "smart_config.h"
#include "metrics/storage_metrics.h"
#include "storage/backends/smart_backend/persistent_sets/unlocked_redblack_tree.h"
#include "storage/backends/smart_backend/persistent_sets/paged_redblack_tree.h"

namespace uh::dbn::storage::smart {

class smart_core {

public:
    explicit smart_core (const smart_config&);

    /**
     * Integrates the data of the given hash
     * @param hash
     * @param data
     * @return effective size
     */
    size_t integrate (std::span <char> hash, std::string_view data);

    /**
     * Retrieves the fragments of the given hash
     * @param hash
     * @return fragments and the total size of data
     */
    std::pair <size_t, std::forward_list <std::span <char>>> retrieve (std::span <char> hash);

private:

    std::pair <std::vector <sets::offset_span>, size_t> deduplicate (std::string_view data);

    uint64_t store_data (const std::string_view& frag);

    static inline size_t largest_common_prefix (const std::string_view &str1, const std::string_view& str2) noexcept;

    fixed_managed_storage m_data_store;
    std::unique_ptr <sets::set_interface> m_fragment_set;
    std::unique_ptr <key_stores::key_store_interface> m_key_store;
    const dedupe_config m_dedupe_conf;
};

} // end namespace uh::dbn::storage::smart

#endif //CORE_SMART_CORE_H
