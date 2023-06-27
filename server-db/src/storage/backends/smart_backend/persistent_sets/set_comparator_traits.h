//
// Created by masi on 6/19/23.
//

#ifndef CORE_SET_COMPARATOR_TRAITS_H
#define CORE_SET_COMPARATOR_TRAITS_H

#include <storage/backends/smart_backend/storage_types/storage_common.h>
#include "index_mem_structures.h"

namespace uh::dbn::storage::smart::sets {


struct set_full_comparator {
    explicit set_full_comparator (managed_storage& storage): m_storage(storage) {}

    [[nodiscard]] inline int operator () (const std::string_view& new_data, const mmap_node& set_data) const {

        const std::string_view data_prefix {reinterpret_cast <const char*> (set_data.data_prefix), sizeof (set_data.data_prefix)};
        if (const auto comp = new_data.compare (data_prefix); comp != 0) {
            return comp;
        }

        auto* p2 = m_storage.get().get_raw_ptr(set_data.m_data.m_data_offset);
        const std::string_view strw2 {static_cast <char*> (p2), set_data.m_data.m_size};
        return new_data.compare(strw2);
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
