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



struct dedupe_set_config {
    std::size_t max_read_cache_size;
    std::size_t moderated_read_cache_free_space;
};

class dedupe_set {

public:

    dedupe_set (dedupe_set_config conf, global_data_view& storage):
        m_storage (storage), m_ioc (m_storage.get().get_executor()), m_dedupe_conf (std::move(conf)) {

    }

private:

    std::reference_wrapper <global_data_view> m_storage;
    std::shared_ptr <boost::asio::io_context> m_ioc;
    dedupe_set_config m_dedupe_conf;

    std::queue <std::reference_wrapper <std::atomic <std::optional <std::string_view>>>> m_cached_refs;
    std::queue <owning_span <char>> m_cache;
    std::atomic <std::size_t> m_cache_size {};

    std::mutex m_cache_alloc_mutex;
    std::shared_mutex m_cache_moderate_mutex;

    struct fragment_element {
        std::reference_wrapper <dedupe_set> m_dedupe_set;
        mutable std::atomic <std::optional <std::string_view>> cached_data;
        uint16_t size;
        uint128_t pointer;
        uint128_t prefix {0};
        mutable std::optional <fragment> max_match;


        fragment_element (const std::string_view& data, dedupe_set& dd_set):
                fragment_element(data, 0, dd_set) {
            max_match.emplace(0, 0);
        }

        fragment_element (const std::string_view& data, const uint128_t& ptr, dedupe_set& dd_set):
                m_dedupe_set(dd_set), cached_data(data), size (data.size()), pointer (ptr), max_match(std::nullopt){
            std::memcpy (prefix.ref_data(), data.data(), std::min (sizeof (uint128_t), data.size()));
        }

        fragment_element (fragment_element&& f) noexcept:
            m_dedupe_set(f.m_dedupe_set),
            cached_data(f.cached_data.load()),
            size (f.size),
            pointer (f.pointer),
            prefix (f.prefix),
            max_match (f.max_match) {
            f.cached_data.store(std::nullopt);
            f.size = 0;
            f.pointer = 0;
            f.prefix = 0;
            f.max_match.reset();
        }

        bool operator == (const fragment_element& f) const {
            if (prefix != f.prefix) [[likely]] {
                return false;
            }

            std::shared_lock <std::shared_mutex> lk (m_dedupe_set.get().m_cache_moderate_mutex);
            m_dedupe_set.get().cache_fragment (this);
            m_dedupe_set.get().cache_fragment (&f);
            update_max(f);
            const auto res = cached_data.load().value() == f.cached_data.load().value();
            lk.unlock();

            m_dedupe_set.get().moderate_cache ();
            return res;
        }

        bool operator <= (const fragment_element& f) const {
            if (prefix != f.prefix) [[likely]] {
                return prefix < f.prefix;
            }

            std::shared_lock <std::shared_mutex> lk (m_dedupe_set.get().m_cache_moderate_mutex);
            m_dedupe_set.get().cache_fragment (this);
            m_dedupe_set.get().cache_fragment (&f);
            update_max(f);
            const auto res = cached_data.load().value() <= f.cached_data.load().value();
            lk.unlock();

            m_dedupe_set.get().moderate_cache ();
            return res;
        }

        bool operator < (const fragment_element& f) const {
            if (prefix != f.prefix) [[likely]] {
                return prefix < f.prefix;
            }

            std::shared_lock <std::shared_mutex> lk (m_dedupe_set.get().m_cache_moderate_mutex);
            m_dedupe_set.get().cache_fragment (this);
            m_dedupe_set.get().cache_fragment (&f);
            update_max(f);
            const auto res = cached_data.load().value() < f.cached_data.load().value();
            lk.unlock();

            m_dedupe_set.get().moderate_cache ();
            return res;
        }

        inline void update_max (const fragment_element& f) const noexcept {
            if (max_match.has_value()) {
                const auto common_prefix = largest_common_prefix(cached_data.load().value(), f.cached_data.load().value());
                if (common_prefix > max_match.value().size) {
                    max_match.emplace(pointer, common_prefix);
                }
            }
            else if (f.max_match.has_value()) {
                const auto common_prefix = largest_common_prefix(cached_data.load().value(), f.cached_data.load().value());
                if (common_prefix > f.max_match.value().size) {
                    f.max_match.emplace(f.pointer, common_prefix);
                }
            }
        }

        static size_t largest_common_prefix (const std::string_view &str1, const std::string_view &str2) noexcept {
            size_t i = 0;
            const auto min_size = std::min (str1.size(), str2.size());
            while (i < min_size and str1[i] == str2[i]) {
                i++;
            }
            return i;
        }
    };

    void moderate_cache () {

        if (m_cache_size > m_dedupe_conf.max_read_cache_size) [[unlikely]] {
            std::lock_guard <std::shared_mutex> lk (m_cache_moderate_mutex);

            while (m_cache_size > m_dedupe_conf.max_read_cache_size - m_dedupe_conf.moderated_read_cache_free_space) {
                m_cache_size -= m_cache.front().size;
                m_cache.pop();
                m_cached_refs.front().get().store(std::nullopt);
                m_cached_refs.pop();
            }
        }
    }

    inline void cache_fragment (const fragment_element* f) {
        std::unique_lock <std::mutex> lk (m_cache_alloc_mutex);

        if (f->cached_data.load().has_value()) {
            return;
        }
        m_cache.emplace(f->size);
        const auto cache_ptr = m_cache.front().data.get();
        f->cached_data.store (std::optional <std::string_view> {std::string_view {cache_ptr, f->size}});
        m_cached_refs.emplace(std::ref (f->cached_data));
        m_cache_size += f->size;

        lk.unlock();

        auto fut = boost::asio::co_spawn (*m_ioc, m_storage.get().read (cache_ptr, f->pointer, f->size), boost::asio::use_future);
        fut.wait();
    }

    std::set <fragment_element> m_set;

public:


    class dedupe_handle {
        std::set <fragment_element>::const_iterator hint;
        std::queue <std::reference_wrapper <std::atomic <std::optional <std::string_view>>>> tmp_cache;
        const std::reference_wrapper <dedupe_set> m_dedupe_set;
    public:
        explicit dedupe_handle (dedupe_set& dd_set): m_dedupe_set(dd_set) {}

        ~dedupe_handle() {
            std::lock_guard <std::shared_mutex> lk (m_dedupe_set.get().m_cache_moderate_mutex);

            while (m_dedupe_set.get().m_cache_size + tmp_cache.front().get().load().value().size() < m_dedupe_set.get().m_dedupe_conf.max_read_cache_size) {
                const auto tmp_data = tmp_cache.front().get().load().value();
                m_dedupe_set.get().m_cache_size += tmp_data.size();
                m_dedupe_set.get().m_cache.emplace(tmp_data.size());
                const auto ptr = m_dedupe_set.get().m_cache.front().data.get();
                std::memcpy (ptr, tmp_data.data(), tmp_data.size());
                //m_dedupe_set.get().m_cached_refs.emplace(std::optional <std::string_view> {std::string_view{ptr,
                                                                                                            //tmp_data.size()}});
                tmp_cache.pop();
            }
        }

        friend dedupe_set;
    };

    friend fragment_element;
    friend dedupe_handle;

    fragment find_similar (std::string_view data, dedupe_handle& h) {
        fragment_element f {data, *this};
        const auto eq = m_set.equal_range(f);
        h.hint = eq.first;
        return f.max_match.value();
    }

    void insert (const uint128_t& pointer, const std::string_view& data, dedupe_handle& h) {
        fragment_element f{data, pointer, *this};
        h.tmp_cache.emplace(std::ref (f.cached_data));
        m_set.emplace_hint(h.hint, std::move(f));
        // TODO write in file
    }
};

} // end namespace uh::cluster

#endif //UH_CLUSTER_DEDUPE_SET_H
