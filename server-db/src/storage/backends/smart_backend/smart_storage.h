//
// Created by masi on 5/22/23.
//

#ifndef CORE_SMART_STORAGE_H
#define CORE_SMART_STORAGE_H

#include "mmap_set.h"
#include "mmap_storage.h"
#include "robin_hood_hashmap.h"

namespace uh::dbn::storage::smart {

char *init_mmap(const std::filesystem::path &file_path, size_t init_size, size_t &file_size);

class smart_storage {

public:
    smart_storage (const std::forward_list<file_mmap_info>& files, std::filesystem::path fragment_set_path, std::filesystem::path hashtable_path);

    size_t integrate (std::span <char> hash, std::string_view data);

    std::span <fragment> retrieve (std::span <char> hash);


private:

    /**
     * stefan's network ioring-based code will invoke this function for each fragment to send out the data
     * @return
     */
    std::span <char> get_fragment_data_ptr (const fragment&);

    std::pair <std::vector <fragment>, size_t> deduplicate (std::string_view data);

    uint64_t store_data (const std::string_view& frag);

    static inline size_t largest_common_prefix (const std::string_view &str1, const std::string_view& str2) noexcept;

    constexpr static size_t MIN_FRAGMENT_SIZE = 4;

    mmap_storage m_data_store;
    mmap_set m_fragment_set;
    robin_hood_hashmap m_hashtable;

};

} // end namespace uh::dbn::storage::smart

#endif //CORE_SMART_STORAGE_H
