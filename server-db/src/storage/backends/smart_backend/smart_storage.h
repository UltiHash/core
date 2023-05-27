//
// Created by masi on 5/22/23.
//

#ifndef CORE_SMART_STORAGE_H
#define CORE_SMART_STORAGE_H

#include "persisted_redblack_tree_set.h"
#include "fixed_managed_storage.h"
#include "persisted_robinhood_hashmap.h"

namespace uh::dbn::storage::smart {

class smart_storage {

public:
    smart_storage (const std::forward_list<file_mmap_info>& data_files,
                   std::filesystem::path fragment_set_path,
                   std::filesystem::path hashtable_path,
                   std::filesystem::path hashtable_value_directory);

    size_t integrate (std::span <char> hash, std::string_view data);

    std::vector <fragment> retrieve (std::span <char> hash);

    std::vector <char> serialize_fragments (const std::vector<fragment>&);

private:

    /**
     * stefan's network ioring-based code will invoke this function for each fragment to send out the data
     * @return
     */
    std::span <char> get_fragment_data_ptr (const fragment&);

    std::pair <std::vector <fragment>, size_t> deduplicate (std::string_view data);

    uint64_t store_data (const std::string_view& frag);

    static inline size_t largest_common_prefix (const std::string_view &str1, const std::string_view& str2) noexcept;

    constexpr static size_t HASH_SIZE = 128;
    constexpr static size_t MIN_FRAGMENT_SIZE = 4;

    fixed_managed_storage m_data_store;
    persisted_redblack_tree_set m_fragment_set;
    persisted_robinhood_hashmap m_hashtable;

};

} // end namespace uh::dbn::storage::smart

#endif //CORE_SMART_STORAGE_H
