//
// Created by masi on 6/19/23.
//

#include "sorted_key_map.h"
namespace uh::dbn::storage::smart::maps {
sorted_key_map::sorted_key_map(sorted_map_config conf):
        m_storage (std::move (conf.data_store)),
        m_set(sets::paged_redblack_tree <sets::set_partial_comparator> (std::move (conf.index_store), m_storage)) {}

void sorted_key_map::insert(std::span<char> key, std::span<char> value, const sets::index_type& index) {

    auto off_span = store_data (key, value);

    std::string_view data = {off_span.first.m_addr, off_span.second};
    m_set.add_pointer(data, off_span.first.m_offset, index);
}

void sorted_key_map::update(std::span<char> key, std::span<char> value, const sets::index_type &pos) {
    auto result = m_set.find({key.data(), key.size()}, pos);
    if (!result.match.has_value()) {
        throw std::logic_error ("The given position for update is not valid!");
    }
    const auto& match = result.match.value();
    m_storage.deallocate(match.data_offset, match.data.size());

    auto off_span = store_data (key, value);

    auto n = m_set.get_node (match.index_offset);
    n.m_mnode->m_data.m_data_offset = off_span.first.m_offset;
    n.m_mnode->m_data.m_size = off_span.second;

}

map_result sorted_key_map::get(std::span<char> key) {
    auto result = m_set.find({key.data(), key.size()});
    return map_result {result};
}

std::list<map_key_value>
sorted_key_map::get_range (const std::span<char> &start_key, const std::span<char> &end_key) {

    auto results = m_set.get_range(start_key, end_key);

    std::list <map_key_value> res;

    for (const auto& result: results) {
        res.emplace_back (result);
    }

    return res;
}

void sorted_key_map::remove(std::span<char> key) {
    throw std::runtime_error ("not implemented");
}

sorted_key_map::~sorted_key_map() {
    m_storage.sync();
    m_set.sync({.position = 0});
}

std::pair<offset_ptr, size_t> sorted_key_map::store_data(std::span<char> key, std::span<char> value) {

    const auto size = key.size() + value.size() + sizeof (uint16_t);
    auto alloc = m_storage.allocate(size);
    uint16_t& key_size = *reinterpret_cast <uint16_t*> (alloc.m_addr);
    key_size = key.size();
    std::memcpy(alloc.m_addr + sizeof (uint16_t), key.data(), key.size());
    std::memcpy(alloc.m_addr + sizeof (uint16_t) + key.size(), value.data(), value.size());
    m_storage.sync(alloc.m_addr, size);
    return {alloc, size};
}


} // end namespace uh::dbn::storage::smart::maps
