#ifndef UH_CLUSTER_DEDUPE_SET_H
#define UH_CLUSTER_DEDUPE_SET_H

#include "common/utils/common.h"
#include "set_log.h"
#include <common/global_data/global_data_view.h>
#include <queue>
#include <set>
#include <utility>

namespace uh::cluster {

class dedupe_set {

public:
    struct fragment_element {
        std::reference_wrapper<global_data_view> m_storage;
        uint128_t pointer;
        uint16_t size{};
        uint128_t prefix{0};
        std::optional<std::string_view> m_data;

        explicit fragment_element(const uint128_t& ptr, uint16_t size_,
                                  const uint128_t& prefix_,
                                  global_data_view& storage)
            : m_storage(storage),
              pointer(ptr),
              size(size_),
              prefix(prefix_),
              m_data(std::nullopt) {}

        fragment_element(const std::string_view& data,
                         global_data_view& storage)
            : fragment_element(data, 0, storage) {
            m_data.emplace(data);
        }

        fragment_element(const std::string_view& data, const uint128_t& ptr,
                         global_data_view& storage)
            : m_storage(storage),
              pointer(ptr),
              size(data.size()),
              m_data(std::nullopt) {
            std::memcpy(prefix.ref_data(), data.data(),
                        std::min(sizeof(uint128_t), data.size()));
        }

        fragment_element(fragment_element&& f) noexcept
            : m_storage(f.m_storage),
              pointer(f.pointer),
              size(f.size),
              prefix(f.prefix),
              m_data(f.m_data) {
            f.size = 0;
            f.pointer = 0;
            f.prefix = 0;
            f.m_data = std::nullopt;
        }

        bool operator<(const fragment_element& f) const {
            if (prefix != f.prefix) [[likely]] {
                return prefix < f.prefix;
            }

            shared_buffer<char> d1, d2;
            std::string_view s1, s2;
            bool b1 = false, b2 = false;

            auto catch_frag = [&](const fragment_element& f,
                                  shared_buffer<char>& data,
                                  std::string_view& str, bool& l1) {
                if (f.m_data.has_value()) {
                    str = *f.m_data;
                } else if (data =
                               m_storage.get().read_l1_cache(f.pointer, f.size);
                           data.data() != nullptr) {
                    l1 = true;
                    str = data.get_str_view();
                } else {
                    data = load_fragment(f, m_storage);
                    str = data.get_str_view();
                }
            };

            catch_frag(*this, d1, s1, b1);
            catch_frag(f, d2, s2, b2);
            size_t comp_len = 0;

            if (b1 or b2) {
                std::string_view s1_l1 = s1;
                std::string_view s2_l1 = s2;
                if (!b1) {
                    s1_l1 = s1.substr(
                        0, std::min(s1.size(),
                                    m_storage.get().l1_cache_sample_size()));
                } else if (!b2) {
                    s2_l1 = s2.substr(
                        0, std::min(s2.size(),
                                    m_storage.get().l1_cache_sample_size()));
                }
                if (const auto comp = s1_l1.compare(s2_l1); comp != 0) {
                    return comp < 0;
                }
                comp_len = std::min(s1_l1.size(), s2_l1.size());
            }

            if (b1 and !m_data.has_value() and
                size > m_storage.get().l1_cache_sample_size()) {
                d1 = load_fragment(*this, m_storage);
                s1 = d1.get_str_view();
            }
            if (b2 and !f.m_data.has_value() and
                f.size > m_storage.get().l1_cache_sample_size()) {
                d2 = load_fragment(f, m_storage);
                s2 = d2.get_str_view();
            }

            return s1.substr(comp_len) < s2.substr(comp_len);
        }
    };

    struct response {
        std::optional<const std::reference_wrapper<const fragment_element>> low;
        std::optional<const std::reference_wrapper<const fragment_element>>
            high;
        const std::set<fragment_element>::const_iterator hint;
    };

    [[nodiscard]] static inline shared_buffer<char>
    load_fragment(const fragment_element& f, global_data_view& storage) {
        if (const auto c = storage.read_l2_cache(f.pointer, f.size);
            c.data() != nullptr) {
            return c;
        }

        shared_buffer<char> buf(f.size);
        storage.read(buf.data(), f.pointer, f.size);
        return buf;
    }

    dedupe_set(const std::filesystem::path& set_log_path,
               global_data_view& storage)
        : m_storage(storage),
          m_set_log(set_log_path) {}

    void load() { m_set_log.replay(m_set, m_storage, m); }

    response find(std::string_view data) {
        fragment_element f{data, m_storage};
        std::shared_lock<std::shared_mutex> lock(m);
        const auto res = m_set.lower_bound(f);
        lock.unlock();

        response resp{.hint = res};
        if (res != m_set.cend()) [[likely]] {
            resp.high.emplace(*res);
        }
        if (res != m_set.cbegin()) [[likely]] {
            resp.low.emplace(*std::prev(res));
        }

        return resp;
    }

    void insert(const uint128_t& pointer, const std::string_view& data,
                const std::set<fragment_element>::const_iterator& hint) {
        fragment_element f{data, pointer, m_storage};
        m_set_log.append({set_operation::INSERT, f.pointer, f.size, f.prefix});
        std::lock_guard<std::shared_mutex> lock(m);
        m_set.emplace_hint(hint, std::move(f));
    }

private:
    global_data_view& m_storage;
    std::set<fragment_element> m_set;
    std::shared_mutex m;
    set_log m_set_log;
};

} // end namespace uh::cluster

#endif // UH_CLUSTER_DEDUPE_SET_H
