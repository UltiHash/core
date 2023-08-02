//
// Created by masi on 6/19/23.
//

#ifndef CORE_SET_COMPARATOR_TRAITS_H
#define CORE_SET_COMPARATOR_TRAITS_H

#include "index_mem_structures.h"
#include "global_data.h"
#include <cstring>

namespace uh::cluster::dedupe  {


struct set_full_comparator {
    explicit set_full_comparator (global_data& storage): m_storage (storage) {}

    [[nodiscard]] inline int operator () (const std::string_view& new_data, const mmap_node& set_data) const {
        if (const auto comp = new_data.substr(0,8).compare({reinterpret_cast <const char*>(&set_data.data_prefix), sizeof(set_data.data_prefix)}); comp != 0) {
            return comp;
        }

        ospan <char> buf (set_data.m_data.m_size);
        m_storage.get().read (buf.data.get(), set_data.m_data.m_data_offset, set_data.m_data.m_size);
        return new_data.compare({buf.data.get(), set_data.m_data.m_size});
    }

    const std::reference_wrapper <global_data> m_storage;
};

struct set_partial_comparator {
    explicit set_partial_comparator (global_data& storage): m_storage(storage) {}

    [[nodiscard]] inline int operator () (const std::string_view& key, const mmap_node& set_data) const {
        throw std::exception ();
        //auto* set_data_p = static_cast <const char*> (m_storage.get().get_raw_ptr(set_data.m_data.m_data_offset));

        //const uint16_t set_key_size = *reinterpret_cast <const uint16_t*> (set_data_p);
        //const std::string_view set_key {set_data_p + sizeof (uint16_t), set_key_size};

        //return key.compare(set_key);
    }

    const std::reference_wrapper <global_data> m_storage;
};

} // end namespace uh::cluster::dedupe

#endif //CORE_SET_COMPARATOR_TRAITS_H
