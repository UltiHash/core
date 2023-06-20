//
// Created by masi on 6/19/23.
//

#include "sorted_key_map.h"
namespace uh::dbn::storage::smart::key_stores {
sorted_key_map::sorted_key_map(set_config set_conf, managed_storage &storage):
        m_set(sets::paged_redblack_tree <sets::set_partial_comparator> (std::move (set_conf), storage)){}

void sorted_key_map::insert(std::span<char> key, std::span<char> value) {

}

std::optional<std::span<char>> sorted_key_map::get(std::span<char> key) {
    return std::optional<std::span<char>>();
}

std::list<std::span<char>>
sorted_key_map::list_keys(const std::span<char> &start_key, const std::span<char> &end_key,
                          const std::span<std::string_view> &labels) {
    return key_store_interface::list_keys(start_key, end_key, labels);
}

void sorted_key_map::remove(std::span<char> key) {

}

sorted_key_map::~sorted_key_map() {

}

} // end namespace uh::dbn::storage::smart::key_stores
