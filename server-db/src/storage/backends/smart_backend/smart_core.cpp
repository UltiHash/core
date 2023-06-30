//
// Created by masi on 5/22/23.
//

#include "smart_core.h"
#include "storage/backends/smart_backend/persistent_maps/sorted_key_map.h"
#include "storage/backends/smart_backend/persistent_maps/persisted_robinhood_hashmap.h"

#include <ranges>


namespace uh::dbn::storage::smart {

smart_core::smart_core (const smart_config& smart_conf):
        m_data_store (smart_conf.data_store_conf),
        m_fragment_set (std::make_unique<sets::paged_redblack_tree <sets::set_full_comparator>>(smart_conf.fragment_set_conf, m_data_store)),
        m_key_store (std::make_unique<maps::sorted_key_map> (std::move (smart_conf.sorted_key_store_config))),
//        m_key_store (std::make_unique<key_stores::persisted_robinhood_hashmap> (std::move (smart_conf.hashmap_key_store_conf))),
        m_dedupe_conf (smart_conf.dedupe_conf) {}

size_t smart_core::integrate(std::span <char> key, std::string_view data) {
    const auto f = m_key_store->get(key);
    if (f.match.has_value()) {
        //TODO should we compare the data as well? It can be that the data
        // is different and we do not notice it
        return 0;
    }

    auto fragments = non_rec_deduplicate (data);
    m_key_store->insert(key, {reinterpret_cast <char*> (fragments.first.data()), fragments.first.size() * sizeof (sets::offset_span)}, f.index);
    return fragments.second;
}

smart_core::fragmented_data smart_core::retrieve(std::span<char> key) {
    auto f = m_key_store->get(key);
    if (f.match.has_value()) {
        return create_fragmented_data(f.match.value().value);
    }
    throw std::out_of_range ("smart_storage could not find the data of the given hash value");
}

std::list <smart_core::key_fragmented_value> smart_core::retrieve_range(std::span<char> start_key, std::span<char> end_key,
                           const std::span<std::string_view> &labels) {
    auto results = m_key_store->get_range(start_key, end_key);

    std::list <smart_core::key_fragmented_value> out;
    for (const auto& result: results) {
        out.push_back({{const_cast <char*> (result.key.data ()), result.key.size ()},
                       create_fragmented_data(result.value)});
    }

    return out;
}

std::pair<std::vector<sets::offset_span>, size_t> smart_core::deduplicate (std::string_view data) {

    const auto f = m_fragment_set->find({data.data(), data.size()});
    if (f.match) {
        return {{{f.match->data_offset, f.match->data.size()}}, 0};
    }

    const auto lower_common_prefix = largest_common_prefix (data, f.lower->data);

    if (lower_common_prefix == data.size()) {
        m_fragment_set->add_pointer (data, f.lower->data_offset, f.index);
        return {{{f.lower->data_offset, data.size()}}, 0};
    }

    const auto upper_common_prefix = largest_common_prefix (data, f.upper->data);
    auto max_common_prefix = upper_common_prefix;
    auto max_data_offset = f.upper->data_offset;
    if (max_common_prefix < lower_common_prefix) {
        max_common_prefix = lower_common_prefix;
        max_data_offset = f.lower->data_offset;
    }

    if (max_common_prefix < m_dedupe_conf.min_fragment_size or data.size() - max_common_prefix < m_dedupe_conf.min_fragment_size) {
        //const auto offset = store_data(data);
        //m_fragment_set->add_pointer (data, offset, f.index);
        //return {{{offset, data.size()}}, data.size()};

        const auto size = std::min (data.size(), m_dedupe_conf.max_fragment_size);
        const auto offset = store_data(data.substr(0, size));
        m_fragment_set->add_pointer (data.substr(0, size), offset, f.index);
        if (data.size() > size) {
            auto fragments = deduplicate(data.substr(size));
            fragments.first.emplace_back(sets::offset_span {offset, size});
            fragments.second += size;
            return fragments;
        }
        else {
            return {{{offset, data.size()}}, data.size()};
        }

    }
    else if (max_common_prefix == data.size()) {
        m_fragment_set->add_pointer (data, max_data_offset, f.index);
        return {{{max_data_offset, data.size()}}, 0};
    }
    else {
        m_fragment_set->add_pointer (data.substr(0, max_common_prefix), max_data_offset, f.index);
        auto fragments = deduplicate (data.substr(max_common_prefix, data.size() - max_common_prefix));
        fragments.first.emplace_back (sets::offset_span {max_data_offset, max_common_prefix});
        return fragments;
    }
}

    std::pair<std::vector<sets::offset_span>, size_t> smart_core::non_rec_deduplicate (std::string_view data) {


        auto integration_data = data;
        std::pair<std::vector<sets::offset_span>, size_t> result;

        while (!integration_data.empty()) {
            const auto f = m_fragment_set->find({integration_data.data(), integration_data.size()});
            if (f.match) {
                result.first.emplace_back(sets::offset_span{f.match->data_offset, integration_data.size()});
                integration_data = integration_data.substr(integration_data.size());
                continue;
            }

            const auto lower_common_prefix = largest_common_prefix (integration_data, f.lower->data);

            if (lower_common_prefix == integration_data.size()) {
                m_fragment_set->add_pointer (integration_data, f.lower->data_offset, f.index);
                result.first.emplace_back(sets::offset_span{f.lower->data_offset, integration_data.size()});
                integration_data = integration_data.substr(integration_data.size());
                continue;
            }

            const auto upper_common_prefix = largest_common_prefix (integration_data, f.upper->data);
            auto max_common_prefix = upper_common_prefix;
            auto max_data_offset = f.upper->data_offset;
            if (max_common_prefix < lower_common_prefix) {
                max_common_prefix = lower_common_prefix;
                max_data_offset = f.lower->data_offset;
            }
            if (max_common < max_common_prefix) {
                max_common = max_common_prefix;
            }
            if (max_common_prefix < m_dedupe_conf.min_fragment_size or integration_data.size() - max_common_prefix < m_dedupe_conf.min_fragment_size) {
                const auto size = std::min (integration_data.size(), m_dedupe_conf.max_fragment_size);
                const auto offset = store_data(integration_data.substr(0, size));
                m_fragment_set->add_pointer (integration_data.substr(0, size), offset, f.index);
                result.first.emplace_back(sets::offset_span{offset, size});
                result.second += size;
                integration_data = integration_data.substr(size);
                continue;
            }
            else if (max_common_prefix == integration_data.size()) {
                m_fragment_set->add_pointer (integration_data, max_data_offset, f.index);
                result.first.emplace_back(sets::offset_span{max_data_offset, integration_data.size()});
                integration_data = integration_data.substr(integration_data.size());
                continue;
            }
            else {
                m_fragment_set->add_pointer (integration_data.substr(0, max_common_prefix), max_data_offset, f.index);
                result.first.emplace_back (sets::offset_span {max_data_offset, max_common_prefix});
                integration_data = integration_data.substr(max_common_prefix, integration_data.size() - max_common_prefix);
                continue;
            }
        }

        return result;
    }


uint64_t smart_core::store_data(const std::string_view& frag) {
    auto alloc = m_data_store.allocate(frag.size());
    std::memcpy(alloc.m_addr, frag.data(), frag.size());
    m_data_store.sync(alloc.m_addr, frag.size());
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

smart_core::fragmented_data smart_core::create_fragmented_data (std::span <const char> fragment_data) {
    const auto ptr = reinterpret_cast <const sets::offset_span*> (fragment_data.data());
    const auto size = fragment_data.size() / sizeof (sets::offset_span);
    size_t total_size = 0;
    std::forward_list <std::span <char>> res;
    for (auto frag = ptr + size - 1; frag >= ptr; frag--) {
        char* raw_ptr = static_cast <char*> (m_data_store.get_raw_ptr(frag->m_data_offset));
        res.emplace_front(raw_ptr, frag->m_size);
        total_size += frag->m_size;
    }
    return {total_size, std::move (res)};
}

} // end namespace uh::dbn::storage::smart
