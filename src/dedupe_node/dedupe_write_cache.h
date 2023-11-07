//
// Created by max on 13.10.23.
//

#ifndef CORE_DEDUPE_WRITE_CACHE_H
#define CORE_DEDUPE_WRITE_CACHE_H

#include <algorithm>
#include "common/common_types.h"
#include "global_data/global_data_view.h"

namespace uh::cluster {

    class dedupe_write_cache {
    public:
        dedupe_write_cache(std::string_view &integration_data, global_data_view &storage, dedupe_config& config) :
                m_integration_data(integration_data), m_storage(storage), m_dedupe_conf(config)
        {

        }

        static inline std::size_t next_divisible(const std::size_t x, const std::size_t y) {
            return (x % y == 0) ? x : x + (y - (x % y));
        }

        [[nodiscard]] std::size_t get_size_available() const {
            return m_cache_available;
        }

        coro <address> write(std::string_view data) {

            //the allocation is deferred until here as the allocate co-routine cannot be executed from the constructor
            if(m_cache_available == 0)
                co_await allocate();

            address addr;

            if(m_cache_available < data.size()) [[unlikely]] {
                // if data is smaller than min_fragment_size, we are better off to live with some unused space
                // as otherwise the metadata for the tiny fragments will take more space
                if(data.size() < m_dedupe_conf.min_fragment_size) {
                    co_await flush();
                } else {
                    auto write_data = data.substr(0, m_cache_available);
                    addr.append_address(co_await write(write_data));
                    data = data.substr(m_cache_available);
                }
            }

            auto write_data = data;

            while (!write_data.empty()) {
                auto frag_free = m_dn_allocation_free.first();
                std::string_view frag_data;
                fragment frag_ptr;

                if(frag_free.size <= write_data.size()) [[unlikely]] {
                    frag_ptr = frag_free;
                    frag_data = write_data.substr(0, frag_ptr.size);

                    write_data = write_data.substr(frag_ptr.size);

                    m_dn_allocation_free.pop_front();
                } else [[likely]] {
                    frag_ptr = frag_free;
                    frag_ptr.size = write_data.size();
                    frag_data = write_data;

                    // set write_data to empty string_view
                    write_data = std::string_view();

                    //update frag_free fragment
                    frag_free.pointer += frag_ptr.size;
                    frag_free.size -= frag_ptr.size;
                    m_dn_allocation_free.set_fragment(0, frag_free);
                }

                if(frag_data.size() != frag_ptr.size) [[unlikely]]
                            throw std::runtime_error("This should never happen");

                std::memcpy(m_cache_data_ptr, frag_data.data(), frag_data.size());
                m_cached_offsets.emplace(frag_ptr.pointer);
                m_cached_map.emplace(std::string_view {m_cache_data_ptr, frag_data.size()}, frag_ptr);
                m_cache_data_ptr += frag_ptr.size;
                m_cache_available -= frag_ptr.size;
                addr.push_fragment(frag_ptr);

                if(m_cache_available == 0) {
                    co_await flush();
                }
            }

            co_return std::move(addr);
        }

        inline bool is_dirty() {
            return m_cache_data.get() != m_cache_data_ptr;
        }

        coro <void> flush() {
            if(!is_dirty())
                co_return;


            co_await m_storage.scatter_allocated_write(m_dn_allocation, {m_cache_data.get(), m_cache_size});
            m_cached_map.clear();
            m_cached_offsets.clear();
            if(m_integration_data.empty()) {
                co_return;
            }

            co_await allocate();

            co_return;
        }

        [[nodiscard]] inline const std::map <std::string_view, fragment>& get_cached_map () const {
            return m_cached_map;
        }

        [[nodiscard]] inline bool has_offset (const uint128_t& offset) const {
            return m_cached_offsets.contains(offset);
        }

    private:
        std::string_view& m_integration_data;
        global_data_view& m_storage;
        dedupe_config& m_dedupe_conf;

        std::size_t m_cache_size = 0;
        std::size_t m_cache_available = 0;
        std::unique_ptr<char[]> m_cache_data;
        char* m_cache_data_ptr;

        address m_dn_allocation;
        address m_dn_allocation_free;
        std::map <std::string_view, fragment> m_cached_map;
        std::set <uint128_t> m_cached_offsets;

        coro <void> allocate() {
            std::size_t new_cache_size = next_divisible(std::min(m_storage.get_data_node_count() * m_dedupe_conf.write_cache_size_per_dn, m_integration_data.size()), m_storage.get_data_node_count());
            if(m_cache_size != new_cache_size) [[unlikely]] {
                m_cache_size = new_cache_size;
                m_cache_data = std::make_unique_for_overwrite<char[]>(m_cache_size);
            }
            m_cache_data_ptr = m_cache_data.get();
            m_cache_available = m_cache_size;
            m_dn_allocation = co_await m_storage.allocate(m_cache_size);
            m_dn_allocation_free = m_dn_allocation;
        }
    };

} // namespace uh::cluster
#endif //CORE_DEDUPE_WRITE_CACHE_H
