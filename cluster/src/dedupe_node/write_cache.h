//
// Created by max on 13.10.23.
//

#ifndef CORE_WRITE_CACHE_H
#define CORE_WRITE_CACHE_H

#include "common/common_types.h"

namespace uh::cluster {

    class write_cache {
    public:
        write_cache(std::size_t requested_size, std::size_t num_nodes, global_data& storage) : m_storage(storage) {
            m_cache_size = std::lcm(std::min(num_nodes * m_cache_size_per_node, requested_size), num_nodes);
        }

        coro <address> write(std::string_view data) {
            //the allocation is deferred until here as the allocate co-routine cannot be executed from the constructor
            if(m_cache_available == 0) {
                m_allocation = co_await m_storage.allocate(m_cache_size);
                m_cache_available = m_cache_size;
            }

            if(m_cache_available < data.size()) [[unlikely]] {
                throw std::out_of_range("The size of a fragment exceeded the capacity of the write cache.");
            }


            auto write_data = data;
            address addr;
            while (!write_data.empty()) {
                auto alloction_frag = m_allocation.first();

                if(alloction_frag.size < write_data.size()) {
                    auto frag_data = write_data.substr(0, alloction_frag.size);
                    m_fragment_cache.insert({alloction_frag, frag_data});
                    m_cache_available -= alloction_frag.size;
                    addr.push_fragment(alloction_frag);

                    write_data = write_data.substr(alloction_frag.size);
                    m_allocation.pop_front();
                } else {
                    auto new_frag = alloction_frag;
                    new_frag.size = write_data.size();
                    m_fragment_cache.insert({new_frag, write_data});
                    m_cache_available -= new_frag.size;
                    addr.push_fragment(new_frag);

                    //update allocation fragment
                    if(alloction_frag.size == new_frag.size) {
                        m_allocation.pop_front();
                    } else {
                        alloction_frag.pointer += new_frag.size;
                        alloction_frag.size -= new_frag.size;
                        m_allocation.set_fragment(0, alloction_frag);
                    }
                    write_data = write_data.substr(0, 0);
                }
            }

            co_return addr;
        }

    private:

        const std::size_t m_cache_size_per_node = 1024 * 1024; // 1MiB
        global_data& m_storage;
        std::size_t m_cache_size;
        std::size_t m_cache_available = 0;
        address m_allocation;
        std::unordered_map<fragment, std::string_view> m_fragment_cache;

    };

}
#endif //CORE_WRITE_CACHE_H
