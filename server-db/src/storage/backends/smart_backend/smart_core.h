//
// Created by masi on 5/22/23.
//

#ifndef CORE_SMART_CORE_H
#define CORE_SMART_CORE_H

#include "storage/backends/smart_backend/storage_types/fixed_managed_storage.h"
#include "smart_config.h"
#include "metrics/storage_metrics.h"
#include "storage/backends/smart_backend/persistent_sets/paged_redblack_tree.h"
#include "storage/backends/smart_backend/persistent_maps//map_interface.h"
#include "util/structured_queries.h"

namespace uh::dbn::storage::smart {

class smart_core {


    typedef std::pair <size_t, std::forward_list <std::span <char>>> fragmented_data;
    typedef std::pair <std::span <char>, fragmented_data> key_fragmented_value;

public:
    explicit smart_core (const smart_config&);

    /**
     * Integrates the data of the given key
     * @param key
     * @param data
     * @return effective size
     */
    std::pair <std::uint8_t, std::size_t> integrate (std::span <char> key, std::string_view data, util::insertion_type insert_type);

    /**
     * Retrieves the fragments of the given key
     * @param key
     * @return fragments and the total size of data
     */
    fragmented_data retrieve (std::span <char> key);

    /**
     * Retrieves the key values with keys within the given range / having the given labels
     */
    std::list <key_fragmented_value> retrieve_range (std::span <char> start_key, std::span <char> end_key, const std::span <std::string_view>& labels);
    size_t max_common = 0;

private:

    std::pair <std::vector <sets::offset_span>, size_t> deduplicate (std::string_view data);

    uint64_t store_data (const std::string_view& frag);

    static inline size_t largest_common_prefix (const std::string_view &str1, const std::string_view& str2) noexcept;

    fragmented_data create_fragmented_data (std::span <const char> fragment_data);

    fixed_managed_storage m_data_store;
    std::unique_ptr <sets::set_interface> m_fragment_set;
    std::unique_ptr <maps::map_interface> m_key_store;
    const dedupe_config m_dedupe_conf;
    std::size_t m_total_effective_size {};
};

} // end namespace uh::dbn::storage::smart

#endif //CORE_SMART_CORE_H
