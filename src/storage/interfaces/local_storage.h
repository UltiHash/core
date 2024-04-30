
#ifndef UH_CLUSTER_LOCAL_STORAGE_H
#define UH_CLUSTER_LOCAL_STORAGE_H

#include "common/utils/pointer_traits.h"
#include "storage/data_store.h"
#include "storage_interface.h"
#include <boost/asio/thread_pool.hpp>
#include <string_view>
#include <utility>

namespace uh::cluster {

struct local_storage : public storage_interface {

    local_storage(uint32_t index, const data_store_config& config,
                  int data_store_count)
        : m_threads(16 * data_store_count) {
        m_data_stores.reserve(data_store_count);
        for (int i = 0; i < data_store_count; i++) {
            m_data_stores.emplace_back(
                std::make_unique<data_store>(config, index, i));
        }
    }

    coro<address> write(const std::string_view& data) override {
        const size_t part =
            std::ceil((double)data.size() / (double)m_data_stores.size());

        address total_addr;
        for (size_t i = 0; i < m_data_stores.size(); ++i) {
            const auto part_size = std::min(data.size() - i * part, part);
            auto addr = m_data_stores[i]->register_write(
                data.substr(i * part, part_size));
            total_addr.append_address(addr);
            boost::asio::post(m_threads, [addr = std::move(addr), i, this]() {
                m_data_stores[i]->perform_write(addr);
            });
        }

        co_return total_addr;
    }

    coro<void> read_fragment(char* buffer, const fragment& f) override {
        get_data_store(f.pointer).read(buffer, f.pointer, f.size);
        co_return;
    }

    coro<void> read_address(char* buffer, const address& addr,
                            const std::vector<size_t>& offsets) override {

        for (size_t i = 0; i < addr.size(); i++) {
            const auto frag = addr.get_fragment(i);
            if (get_data_store(frag.pointer)
                    .read(buffer + offsets[i], frag.pointer, frag.size) !=
                frag.size) [[unlikely]] {
                throw std::runtime_error(
                    "Could not read the data with the given size");
            }
        }
        co_return;
    }

    coro<void> sync(const address& addr) override {

        std::vector<address> ds_addresses(m_data_stores.size());
        for (size_t i = 0; i < addr.size(); i++) {
            const auto f = addr.get_fragment(i);
            const auto id = pointer_traits::get_data_store_id(f.pointer);
            ds_addresses.at(id).push_fragment(f);
        }

        std::vector<std::future<void>> futures;
        futures.reserve(m_data_stores.size());
        for (size_t i = 0; i < m_data_stores.size(); ++i) {

            auto p = std::make_shared<std::promise<void>>();
            boost::asio::post(m_threads, [i, this, p, &ds_addresses]() {
                try {
                    m_data_stores[i]->wait_for_ongoing_writes(ds_addresses[i]);
                    m_data_stores[i]->sync();
                    p->set_value();
                } catch (const std::exception&) {
                    p->set_exception(std::current_exception());
                }
            });
            futures.emplace_back(p->get_future());
        }
        for (auto& f : futures) {
            f.get();
        }
        co_return;
    }

    coro<size_t> get_used_space() override {
        size_t used = 0;
        for (const auto& ds : m_data_stores) {
            used += ds->get_used_space();
        }
        co_return used;
    }

    coro<size_t> get_free_space() override {
        size_t free = 0;
        for (const auto& ds : m_data_stores) {
            free += ds->get_available_space();
        }
        co_return free;
    }

private:
    std::vector<std::unique_ptr<data_store>> m_data_stores;
    boost::asio::thread_pool m_threads;

    data_store& get_data_store(const uint128_t& pointer) {
        return *m_data_stores[pointer_traits::get_data_store_id(pointer)];
    }
};

} // namespace uh::cluster

#endif // UH_CLUSTER_LOCAL_STORAGE_H
