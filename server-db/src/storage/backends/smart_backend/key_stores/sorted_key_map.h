//
// Created by masi on 6/19/23.
//

#ifndef CORE_SORTED_KEY_MAP_H
#define CORE_SORTED_KEY_MAP_H

#include "key_store_interface.h"
#include <storage/backends/smart_backend/persistent_sets/paged_redblack_tree.h>

namespace uh::dbn::storage::smart::key_stores {

class sorted_key_map: public key_store_interface {

    sorted_key_map (set_config set_conf, managed_storage& storage);

    void insert (std::span <char> key, std::span <char> value) override;

    std::optional <std::span <char>> get (std::span <char> key) override;

    std::list<std::span<char>> list_keys (const std::span<char> &start_key, const std::span<char> &end_key, const std::span<std::string_view> &labels) override;

    void remove (std::span <char> key) override;

    ~sorted_key_map () override;

    sets::paged_redblack_tree <sets::set_partial_comparator> m_set;

};

} // end namespace uh::dbn::storage::smart::key_stores

#endif //CORE_SORTED_KEY_MAP_H
