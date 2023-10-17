//
// Created by max on 13.10.23.
//

#ifndef CORE_DEDUPE_WRITE_CACHE_H
#define CORE_DEDUPE_WRITE_CACHE_H

#include "common/common_types.h"
#include "dedupe_node/global_data.h"

namespace uh::cluster {

    class dedupe_write_cache {
    //todo: make sure allocated storage on the data node is freed in case of an exception -> avoid storage data leaks
    public:
        dedupe_write_cache(std::string_view &integration_data, global_data &storage, dedupe_config& config) :
        m_integration_data(integration_data), m_storage(storage), m_dedupe_conf(config) {
            //todo: m_cache_size can now divert from
            m_cache_size = std::lcm(std::min(m_storage.get_data_node_count() * m_cache_size_per_node, m_integration_data.size()), m_storage.get_data_node_count());
            m_cache_data = static_cast<char *>(std::malloc(m_cache_size));
            m_cache_data_ptr = m_cache_data;
        }

        ~dedupe_write_cache() {
            if(m_cache_data != nullptr) {
                std::free(m_cache_data);
            }
        }

        std::size_t get_size_available() {
            return m_cache_available;
        }

        coro <address> write(std::string_view data) {
            // todo: the write method has to make sure fragments are contained within the buffer partition belonging
            // todo: a single data node. Otherwise, the data belonging to a fragment may be split across nodes without
            // todo: reflecting this fragment split in the fragment/address meta-data --> data corruption
            //the allocation is deferred until here as the allocate co-routine cannot be executed from the constructor
            if(m_cache_available == 0) {
                m_dn_allocation = co_await m_storage.allocate(m_cache_size);
                m_cache_available = m_cache_size;
            }

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
                auto frag_free = m_dn_allocation.first();
                std::string_view frag_data;
                fragment frag_ptr;

                if(frag_free.size < write_data.size()) {
                    frag_ptr = frag_free;
                    frag_data = write_data.substr(0, frag_ptr.size);

                    write_data = write_data.substr(frag_free.size);

                    m_dn_allocation.pop_front();
                } else {
                    frag_ptr = frag_free;
                    frag_ptr.size = write_data.size();
                    frag_data = write_data;

                    write_data = write_data.substr(0, 0);

                    //update frag_free fragment
                    if(frag_free.size == frag_ptr.size) {
                        m_dn_allocation.pop_front();
                    } else {
                        frag_free.pointer += frag_ptr.size;
                        frag_free.size -= frag_ptr.size;
                        m_dn_allocation.set_fragment(0, frag_free);
                    }
                }

                m_cache_fragments.push_fragment(frag_ptr);
                std::memcpy(m_cache_data_ptr, frag_data.data(), frag_data.size());
                m_cache_data_ptr += frag_data.size();

                addr.push_fragment(frag_ptr);

                if(m_cache_available == 0) {
                    co_await flush();
                }
            }

            co_return std::move(addr);
        }

        void cleanup() {

        }

        coro <void> flush() {
            if(m_cache_fragments.empty() && m_cache_data == m_cache_data_ptr) {
                co_return;
            }

            co_await m_storage.scatter_allocated_write(m_cache_fragments, {m_cache_data, m_cache_size});

            if(m_integration_data.empty()) {
                m_cache_size = 0;
                m_cache_available = 0;
                std::free(m_cache_data);
                m_cache_data = nullptr;
                m_cache_data_ptr = nullptr;

                co_return;
            }

            //clear internal buffer, resize if necessary and reset all bookkeepers
            std::memset(m_cache_data, 0, m_cache_size);
            std::size_t new_cache_size = std::lcm(std::min(m_storage.get_data_node_count() * m_cache_size_per_node, m_integration_data.size()), m_storage.get_data_node_count());
            if(m_cache_size != new_cache_size) [[unlikely]] {
                std::free(m_cache_data);
                m_cache_data = static_cast<char *>(std::malloc(new_cache_size));
                m_cache_size = new_cache_size;
            }
            m_cache_data_ptr = m_cache_data;

            //acquire new data node allocation
            m_dn_allocation = co_await m_storage.allocate(m_cache_size);
            m_cache_available = m_cache_size;

            m_cache_fragments = address();

            co_return;
        }

    private:

        const std::size_t m_cache_size_per_node = 1024 * 1024; // 1MiB

        std::string_view& m_integration_data;
        global_data& m_storage;
        dedupe_config& m_dedupe_conf;

        address m_cache_fragments;
        std::size_t m_cache_size;
        std::size_t m_cache_available = 0;
        char* m_cache_data;
        char* m_cache_data_ptr;

        address m_dn_allocation;
    };

}
#endif //CORE_DEDUPE_WRITE_CACHE_H
