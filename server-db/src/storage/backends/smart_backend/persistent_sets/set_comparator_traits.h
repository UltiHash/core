//
// Created by masi on 6/19/23.
//

#ifndef CORE_SET_COMPARATOR_TRAITS_H
#define CORE_SET_COMPARATOR_TRAITS_H

#include <storage/backends/smart_backend/storage_types/storage_common.h>
#include "index_mem_structures.h"

#include <cstring>

namespace uh::dbn::storage::smart::sets {


struct set_full_comparator {
    explicit set_full_comparator (managed_storage& storage): m_storage(storage) {}

    [[nodiscard]] inline int operator () (const std::string_view& new_data, const mmap_node& set_data) const {

        //if (const auto comp = memcmp (new_data.data(), &set_data.data_prefix, std::min (sizeof(uint64_t), new_data.size())); comp != 0) {
       //     return comp;
        //}

        //const uint64_t &data_prefix = *reinterpret_cast <const uint64_t *> (new_data.data());
        //int comp = data_prefix - set_data.data_prefix;
        //if (const int comp = set_data.data_prefix - data_prefix; comp != 0) {
        //    return comp;
        //}

        const auto* p2 = m_storage.get().get_raw_ptr(set_data.m_data.m_data_offset);

        const std::string_view set_content {static_cast <const char*> (p2), set_data.m_data.m_size};
        return new_data.compare(set_content);
    }

    const std::reference_wrapper <managed_storage> m_storage;
};

struct set_partial_comparator {
    explicit set_partial_comparator (managed_storage& storage): m_storage(storage) {}

    [[nodiscard]] inline int operator () (const std::string_view& key, const mmap_node& set_data) const {

        auto* set_data_p = static_cast <const char*> (m_storage.get().get_raw_ptr(set_data.m_data.m_data_offset));

        const uint16_t set_key_size = *reinterpret_cast <const uint16_t*> (set_data_p);
        const std::string_view set_key {set_data_p + sizeof (uint16_t), set_key_size};

        return key.compare(set_key);
    }

    const std::reference_wrapper <managed_storage> m_storage;
};

} // end namespace uh::dbn::storage::smart::sets

#endif //CORE_SET_COMPARATOR_TRAITS_H
