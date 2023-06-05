//
// Created by masi on 5/22/23.
//

#include "smart_core.h"

#include <ranges>


namespace uh::dbn::storage::smart {

smart_core::smart_core (const smart_config& smart_conf):
        m_data_store (smart_conf.data_store_conf),
        m_fragment_set (smart_conf.set_conf, m_data_store),
        m_hashtable (smart_conf.map_conf),
        m_dedupe_conf (smart_conf.dedupe_conf) {}

size_t smart_core::integrate(std::span <char> hash, std::string_view data) {

    if (const auto f = m_hashtable.get(hash); f) {
        //TODO should we compare the data as well? It can be that the data
        // is different and we do not notice it
        return 0;
    }

    auto fragments = deduplicate (data);
    m_hashtable.insert(hash, {reinterpret_cast <char*> (fragments.first.data()), fragments.first.size() * sizeof (fragment)});
    return fragments.second;
}

std::pair <size_t, std::forward_list <std::span <char>>> smart_core::retrieve(std::span<char> hash) {
    auto f = m_hashtable.get(hash);
    if (f) {
        const auto ptr = reinterpret_cast <fragment*> (f.value().data());
        const auto size = f.value().size() / sizeof (fragment);
        size_t total_size = 0;
        std::forward_list <std::span <char>> res;
        for (auto frag = ptr; frag < ptr + size; frag++) {
            char* raw_ptr = static_cast <char*> (m_data_store.get_raw_ptr(frag->m_data_offset));
            res.emplace_front(raw_ptr, frag->m_size);
            total_size += frag->m_size;
        }
        return {total_size, res};
    }
    throw std::out_of_range ("smart_storage could not find the data of the given hash value");
}

std::pair<std::vector<fragment>, size_t> smart_core::deduplicate (std::string_view data) {

    const auto f = m_fragment_set.find({data.data(), data.size()});
    if (f.match) {
        return {{{f.match->first, f.match->second.size()}}, 0};
    }

    const auto lower_common_prefix = largest_common_prefix (data, f.lower->second);

    if (lower_common_prefix == data.size()) {
        m_fragment_set.insert_index (data, f.lower->first, f);
        return {{{f.lower->first, data.size()}}, 0};
    }

    const auto upper_common_prefix = largest_common_prefix (data, f.upper->second);
    auto max_common_prefix = upper_common_prefix;
    auto max_data_offset = f.upper->first;
    if (max_common_prefix < lower_common_prefix) {
        max_common_prefix = lower_common_prefix;
        max_data_offset = f.lower->first;
    }

    if (max_common_prefix < m_dedupe_conf.min_fragment_size or data.size() - max_common_prefix < m_dedupe_conf.min_fragment_size) {
        const auto offset = store_data(data);
        m_fragment_set.insert_index(data, offset, f);
        return {{{offset, data.size()}}, data.size()};
    }
    else if (max_common_prefix == data.size()) {
        m_fragment_set.insert_index (data, max_data_offset, f);
        return {{{max_data_offset, data.size()}}, 0};
    }
    else {
        m_fragment_set.insert_index (data.substr(0, max_common_prefix), max_data_offset, f); // but not really add the data
        auto fragments = deduplicate (data.substr(max_common_prefix, data.size() - max_common_prefix));
        fragments.first.emplace_back (max_data_offset, max_common_prefix);
        return fragments;
    }
}

uint64_t smart_core::store_data(const std::string_view& frag) {
    auto alloc = m_data_store.allocate(frag.size());
    std::memcpy(alloc.m_addr, frag.data(), frag.size());
    return alloc.m_offset;
}

size_t
smart_core::largest_common_prefix(const std::string_view &str1, const std::string_view &str2) noexcept {
    size_t i = 0;
    const auto min_size = std::min (str1.size(), str2.size());
    while (i < min_size and str1[i] == str2[i]) {
        i++;
    }
    return i;
}

} // end namespace uh::dbn::storage::smart
