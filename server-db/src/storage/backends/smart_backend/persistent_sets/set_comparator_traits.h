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

    [[nodiscard]] inline int operator () (const std::string_view& new_data, const offset_span& set_data) const {
        auto* p2 = m_storage.get().get_raw_ptr(set_data.m_data_offset);
        const std::string_view strw2 {static_cast <char*> (p2), set_data.m_size};
        return new_data.compare(strw2);
    }

    const std::reference_wrapper <managed_storage> m_storage;
};

struct set_partial_comparator {
    explicit set_partial_comparator (managed_storage& storage): m_storage(storage) {}

    [[nodiscard]] inline int operator () (const std::string_view& new_data, const offset_span& set_data) const {

        const uint16_t new_key_size = *reinterpret_cast <const uint16_t*> (new_data.data());
        const std::string_view new_key {new_data.data () + sizeof (uint16_t), new_key_size};

        auto* set_data_p = static_cast <const char*> (m_storage.get().get_raw_ptr(set_data.m_data_offset));

        const uint16_t set_key_size = *reinterpret_cast <const uint16_t*> (set_data_p);
        const std::string_view set_key {set_data_p + sizeof (uint16_t), set_key_size};

        return new_key.compare(set_key);
    }

    const std::reference_wrapper <managed_storage> m_storage;
};

} // end namespace uh::dbn::storage::smart::sets

#endif //CORE_SET_COMPARATOR_TRAITS_H
