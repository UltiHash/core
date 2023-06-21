//
// Created by masi on 6/19/23.
//

#ifndef CORE_SORTED_KEY_MAP_H
#define CORE_SORTED_KEY_MAP_H

#include "map_interface.h"
#include <storage/backends/smart_backend/storage_types/growing_managed_storage.h>
#include <storage/backends/smart_backend/persistent_sets/paged_redblack_tree.h>

namespace uh::dbn::storage::smart::maps {

class sorted_key_map: public map_interface {
public:
    explicit sorted_key_map (sorted_map_config conf);

    void insert (std::span <char> key, std::span <char> value, const index_type& index) override;

    map_result get (std::span <char> key) override;

    std::list<map_key_value> get_range (const std::span<char> &start_key, const std::span<char> &end_key) override;

    void remove (std::span <char> key) override;

    ~sorted_key_map () override;

    growing_managed_storage m_storage;
    sets::paged_redblack_tree <sets::set_partial_comparator> m_set;

};

} // end namespace uh::dbn::storage::smart::key_stores

#endif //CORE_SORTED_KEY_MAP_H
