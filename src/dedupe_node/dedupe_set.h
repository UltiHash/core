//
// Created by masi on 11/7/23.
//

#ifndef UH_CLUSTER_DEDUPE_SET_H
#define UH_CLUSTER_DEDUPE_SET_H

#include <set>
#include <queue>
#include <utility>
#include <common/common.h>
#include <global_data/global_data_view.h>

namespace uh::cluster {

class dedupe_set {

private:

    struct fragment_element {
        global_data_view& m_storage;
        uint16_t size;
        uint128_t pointer;
        uint128_t prefix {0};
        std::optional <std::string_view> m_data;

        fragment_element (const std::string_view& data, global_data_view& storage):
                fragment_element(data, 0, storage) {
            m_data.emplace(data);
        }

        fragment_element (const std::string_view& data, const uint128_t& ptr, global_data_view& storage):
                m_storage (storage), size (data.size()), pointer (ptr) , m_data (std::nullopt) {
            std::memcpy (prefix.ref_data(), data.data(), std::min (sizeof (uint128_t), data.size()));
        }

        fragment_element (fragment_element&& f) noexcept:
            m_storage (f.m_storage),
            size (f.size),
            pointer (f.pointer),
            prefix (f.prefix),
            m_data (f.m_data) {
            f.size = 0;
            f.pointer = 0;
            f.prefix = 0;
            f.m_data = std::nullopt;
        }

        bool operator < (const fragment_element& f) const {
            if (prefix != f.prefix) [[likely]] {
                return prefix < f.prefix;
            }
            ospan <char> b1, b2;
            std::string_view s1, s2;
            if (!m_data.has_value()) {
                b1 = load_fragment(*this, m_storage);
                s1 = b1.get_str_view();
            }
            else {
                s1 = m_data.value();
            }

            if (!f.m_data.has_value()) {
                b2 = load_fragment(f, m_storage);
                s2 = b2.get_str_view();
            }
            else {
                s2 = f.m_data.value();
            }

            return s1 < s2;
        }

    };

    struct response {
        std::optional <const std::reference_wrapper <const fragment_element>> low;
        std::optional <const std::reference_wrapper <const fragment_element>> up;
        const std::set <fragment_element>::const_iterator hint;
    };

    global_data_view& m_storage;
    std::set <fragment_element> m_set;

public:

    [[nodiscard]] static inline ospan<char> load_fragment (const fragment_element& f, global_data_view& storage) {
        if (auto c = storage.read_cache(f.pointer, f.size); c.has_value()) {
            return std::move (c.value());
        }
        ospan<char> buf (f.size);
        auto fut = boost::asio::co_spawn (*storage.get_executor(), storage.read (buf.data.get(), f.pointer, f.size), boost::asio::use_future);
        fut.wait();
        return buf;
    }

    explicit dedupe_set (global_data_view& storage):
            m_storage (storage) {
    }

    response find (std::string_view data) {
        fragment_element f {data, m_storage};
        const auto res = m_set.lower_bound(f);
        response resp {.hint = res};
        if (res != m_set.cend()) [[likely]] {
            resp.up.emplace(*res);
        }
        if (res != m_set.cbegin()) {
            resp.low.emplace(*std::prev (res));
        }
        return resp;
    }

    void insert (const uint128_t& pointer, const std::string_view& data, const std::set <fragment_element>::const_iterator& hint) {
        fragment_element f{data, pointer, m_storage};
        m_set.emplace_hint(hint, std::move(f));
        // TODO write in file
    }
};

} // end namespace uh::cluster

#endif //UH_CLUSTER_DEDUPE_SET_H
