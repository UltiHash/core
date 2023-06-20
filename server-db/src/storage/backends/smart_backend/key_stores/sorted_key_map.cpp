//
// Created by masi on 6/19/23.
//

#include "sorted_key_map.h"
namespace uh::dbn::storage::smart::key_stores {
sorted_key_map::sorted_key_map(sorted_map_config conf):
        m_storage (std::move (conf.data_store)),
        m_set(sets::paged_redblack_tree <sets::set_partial_comparator> (std::move (conf.index_store), m_storage)) {}

void sorted_key_map::insert(std::span<char> key, std::span<char> value, const sets::index_type& index) {
    const auto size = key.size() + value.size() + sizeof (uint16_t);
    auto alloc = m_storage.allocate(size);
    uint16_t& key_size = *reinterpret_cast <uint16_t*> (alloc.m_addr);
    key_size = key.size();
    std::memcpy(alloc.m_addr + sizeof (uint16_t), key.data(), key.size());
    std::memcpy(alloc.m_addr + sizeof (uint16_t) + key.size(), value.data(), value.size());
    std::string_view data = {alloc.m_addr, size};
    m_set.add_pointer(data, alloc.m_offset, index);
}

map_result sorted_key_map::get(std::span<char> key) {
    auto result = m_set.find({key.data(), key.size()});
    if (result.match.has_value()) {
        const auto data_offset = sizeof (uint16_t) + key.size();
        return {{std::span <const char> (result.match.value().second.data() + data_offset, result.match.value().second.size() - data_offset)}, result.index};
    }
    return {std::nullopt, result.index};
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
