//
// Created by masi on 5/22/23.
//

#include "prefix_deduplicator.h"

namespace uh::dbn::storage::smart {


prefix_deduplicator::prefix_deduplicator(mmap_storage &data_storage, std::filesystem::path fragment_set_path) :
        m_fragment_set (data_storage, std::move (fragment_set_path)){

}

std::pair<std::vector<fragment>, size_t> prefix_deduplicator::deduplicate(std::string_view data) {

    const auto res = m_fragment_set.find({data.data(), data.size()});
    if (res.match) {
        return {{{res.match->first, res.match->second.size()}}, 0};
    }

    const auto lower_common_prefix = largest_common_prefix (data, res.lower->second);

    if (lower_common_prefix == data.size()) {
        const auto sub_hint = m_fragment_set.insert_index (data, res.lower->first, res.hint);
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
        const auto offset = m_fragment_set.insert_data (data);
        m_fragment_set.insert_index(data, offset, res.hint);
        return {{{offset, data.size()}}, data.size()};
    }
    else if (max_common_prefix == data.size()) {
        const auto sub_hint = m_fragment_set.insert_index (data, max_data_offset, res.hint);
        return {{{max_data_offset, data.size()}}, 0};
    }
    else {
        const auto sub_hint = m_fragment_set.insert_index (data.substr(0, max_common_prefix), max_data_offset, res.hint); // but not really add the data
        auto fragments = deduplicate (data.substr (max_common_prefix, data.size() -  max_common_prefix));
        fragments.first.emplace_back (max_data_offset, max_common_prefix);
        return fragments;
    }
}

size_t
prefix_deduplicator::largest_common_prefix(const std::string_view &str1, const std::string_view &str2) noexcept {
    size_t i = 0;
    const auto min_size = std::min (str1.size(), str2.size());
    while (i < min_size and str1[i] == str2[i]) {
        i++;
    }
    return i;
}

} // end namespace uh::dbn::storage::smart
