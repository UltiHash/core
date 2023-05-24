//
// Created by masi on 5/22/23.
//

#include "smart_storage.h"


namespace uh::dbn::storage::smart {

char *init_mmap(const std::filesystem::path &file_path, size_t init_size, size_t &file_size) {
    const auto existing_index = std::filesystem::exists(file_path.c_str());
    const auto fd = open(file_path.c_str(), O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if (!existing_index) {
        ftruncate(fd, file_size);
    }
    else {
        file_size = lseek (fd, 0, SEEK_END);
    }

    const auto mmap_addr = static_cast <char*> (mmap(nullptr, file_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
    close(fd);

    if (!existing_index) {
        std::memset (mmap_addr, 0, init_size);
    }

    return mmap_addr;
}

} // end namespace uh::dbn::storage::smart

namespace uh::dbn::storage::smart {


smart_storage::smart_storage(const std::forward_list<file_mmap_info>& files, std::filesystem::path fragment_set_path) :
        m_data_store (files),
        m_fragment_set (m_data_store, std::move (fragment_set_path)){

}

size_t smart_storage::integrate(std::span <char> hash, std::string_view data) {

    const auto fragments = deduplicate (data);
    // here we insert into hash
    return fragments.second;
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
