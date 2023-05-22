//
// Created by masi on 5/22/23.
//

#ifndef CORE_PREFIX_DEDUPLICATOR_H
#define CORE_PREFIX_DEDUPLICATOR_H

#include "mmap_set.h"
#include "mmap_storage.h"

namespace uh::dbn::storage::smart {

class prefix_deduplicator {

public:
    prefix_deduplicator (mmap_storage& data_storage, std::filesystem::path fragment_set_path);

    std::pair <std::vector <fragment>, size_t> deduplicate (std::string_view data);

private:

    static inline size_t largest_common_prefix (const std::string_view &str1, const std::string_view& str2) noexcept;

    constexpr static size_t MIN_FRAGMENT_SIZE = 4;

    mmap_set m_fragment_set;

};

} // end namespace uh::dbn::storage::smart

#endif //CORE_PREFIX_DEDUPLICATOR_H
