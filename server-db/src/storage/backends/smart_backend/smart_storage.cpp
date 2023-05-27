//
// Created by masi on 5/22/23.
//

#include "smart_storage.h"

#include <ranges>


namespace uh::dbn::storage::smart {

smart_storage::smart_storage(const std::forward_list<file_mmap_info>& data_files,
                             std::filesystem::path fragment_set_path,
                             std::filesystem::path hashtable_index_path,
                             std::filesystem::path hashtable_value_directory) :
        m_data_store (data_files),
        m_fragment_set (m_data_store, std::move (fragment_set_path)),
        m_hashtable (HASH_SIZE, std::move (hashtable_index_path), std::move (hashtable_value_directory)){

}

size_t smart_storage::integrate(std::span <char> hash, std::string_view data) {

    if (const auto f = m_hashtable.get(hash); f) {
        //TODO should we compare the data as well? It can be that the data
        // is different and we do not notice it
        return 0;
    }

    auto fragments = deduplicate (data);
    m_hashtable.insert(hash, {reinterpret_cast <char*> (fragments.first.data()), fragments.first.size() * sizeof (fragment)});
    return fragments.second;
}

std::vector<fragment> smart_storage::retrieve(std::span<char> hash) {
    auto f = m_hashtable.get(hash);
    if (f) {
        const auto ptr = reinterpret_cast <fragment*> (f.value().data());
        const auto size = f.value().size() / sizeof (fragment);
        return std::vector <fragment> {ptr, ptr + size};
    }
    throw std::out_of_range ("smart_storage could not find the data of the given hash value");
}

std::vector<char> smart_storage::serialize_fragments(const std::vector<fragment>& fragments) {

    size_t offset = 0;
    for (const auto f: fragments) {
        offset += f.m_size;
    }
    std::vector <char> data (offset);

    offset = 0;
    for (auto itr = fragments.crbegin(); itr != fragments.crend(); itr ++) {
        const auto& f = *itr;
        const auto frag_data = get_fragment_data_ptr(f);
        std::memcpy(data.data() + offset, frag_data.data(), f.m_size);
        offset += f.m_size;
    }
    return data;
}

std::span <char> smart_storage::get_fragment_data_ptr (const fragment& frag) {
    char* ptr = static_cast <char*> (m_data_store.get_raw_ptr(frag.m_data_offset));
    return {ptr, frag.m_size};
}


std::pair<std::vector<fragment>, size_t> smart_storage::deduplicate (std::string_view data) {

    const auto res = m_fragment_set.find({data.data(), data.size()});
    if (res.match) {
        return {{{res.match->first, res.match->second.size()}}, 0};
    }

    const auto lower_common_prefix = largest_common_prefix (data, res.lower->second);

    if (lower_common_prefix == data.size()) {
        m_fragment_set.insert_index (data, res.lower->first, res.hint);
        return {{{res.lower->first, data.size()}}, 0};
    }

    const auto upper_common_prefix = largest_common_prefix (data, res.upper->second);
    auto max_common_prefix = upper_common_prefix;
    auto max_data_offset = res.upper->first;
    if (max_common_prefix < lower_common_prefix) {
        max_common_prefix = lower_common_prefix;
        max_data_offset = res.lower->first;
    }

    if (max_common_prefix < MIN_FRAGMENT_SIZE or data.size() - max_common_prefix < MIN_FRAGMENT_SIZE) {
        const auto offset = store_data(data);
        m_fragment_set.insert_index(data, offset, res.hint);
        return {{{offset, data.size()}}, data.size()};
    }
    else if (max_common_prefix == data.size()) {
        m_fragment_set.insert_index (data, max_data_offset, res.hint);
        return {{{max_data_offset, data.size()}}, 0};
    }
    else {
        m_fragment_set.insert_index (data.substr(0, max_common_prefix), max_data_offset, res.hint); // but not really add the data
        auto fragments = deduplicate (data.substr(max_common_prefix, data.size() - max_common_prefix));
        fragments.first.emplace_back (max_data_offset, max_common_prefix);
        return fragments;
    }
}

uint64_t smart_storage::store_data(const std::string_view& frag) {
    auto alloc = m_data_store.allocate(frag.size());
    std::memcpy(alloc.m_addr, frag.data(), frag.size());
    return alloc.m_offset;
}

size_t
smart_storage::largest_common_prefix(const std::string_view &str1, const std::string_view &str2) noexcept {
    size_t i = 0;
    const auto min_size = std::min (str1.size(), str2.size());
    while (i < min_size and str1[i] == str2[i]) {
        i++;
    }
    return i;
}


} // end namespace uh::dbn::storage::smart
