#pragma once

#include <proxy/cache/disk/body.h>
#include <proxy/cache/disk/deletion_queue.h>

#include <proxy/cache/lfu_cache.h>
#include <proxy/cache/lru_cache.h>

#include <common/coroutines/coro_util.h>
#include <storage/global/data_view.h>

#include <memory>
#include <string>

#include <iostream>

namespace uh::cluster::proxy::cache::disk {

class manager {
public:
    using cache_interface = cache_interface<object_metadata, object_handle>;
    using lru_cache = lru_cache<object_metadata, object_handle>;
    using lfu_cache = lfu_cache<object_metadata, object_handle>;
    using deletion_queue = deletion_queue<object_metadata, object_handle>;

    using data_view = storage::data_view;
    using stream = ep::http::stream;
    using body = ep::http::body;

    /*
     * Store object handle in cache
     *
     * It removed address information from the given body.
     */
    coro<void> put(object_metadata& key, reader_body& body) {
        auto objh = body.get_object_handle();
        auto obj_size = objh.data_size();

        std::cout << obj_size << std::endl;

        auto total_size =
            m_current_size.load(std::memory_order_acquire) + obj_size;
        auto threshold = m_capacity - m_eviction_margin;

        auto required_size =
            total_size > threshold ? total_size - threshold : 0;
        std::cout << "required_size: " << required_size
                  << ", obj_size: " << obj_size << std::endl;
        if (required_size > 0) {
            auto evicted = m_cache->evict(required_size);
            auto addr = gather_address(evicted);
            co_await utils::erase(m_storage, addr);

            // apply size change "after ERASE", "before PUT"
            std::cout << "addr.data_size(): " << addr.data_size()
                      << ", obj_size: " << obj_size << std::endl;
            if (addr.data_size() > obj_size) {
                m_current_size.fetch_sub(addr.data_size() - obj_size,
                                         std::memory_order_acq_rel);
            } else {
                m_current_size.fetch_add(obj_size - addr.data_size(),
                                         std::memory_order_acq_rel);
            }
        } else {
            m_current_size.fetch_add(obj_size, std::memory_order_acq_rel);
        }

        auto p_prev = m_cache->put(key, std::move(objh));
        if (p_prev) {
            m_deletion_queue.push(std::move(p_prev));
        }
        std::cout << "Total size after put: " << m_current_size << std::endl;
    }

    std::optional<writer_body> get(object_metadata key) {
        auto entry = m_cache->get(key);
        if (!entry) {
            return std::nullopt;
        }
        return writer_body{m_storage, std::move(entry)};
    }

    static manager create(boost::asio::io_context& ioc, data_view& storage,
                          std::size_t capacity,
                          std::size_t eviction_margin = 0) {
        return manager(ioc, storage, std::make_unique<lru_cache>(), capacity,
                       eviction_margin);
    }

private:
    data_view& m_storage;
    std::unique_ptr<cache_interface> m_cache;
    std::size_t m_capacity;
    std::size_t m_eviction_margin;
    std::atomic<std::size_t> m_current_size{0};

    deletion_queue m_deletion_queue;
    // TODO: spawn a background task to remove
    coro_task m_task;

    manager(boost::asio::io_context& ioc, data_view& storage,
            std::unique_ptr<cache_interface> c, std::size_t capacity,
            std::size_t eviction_margin)
        : m_storage{storage},
          m_cache{std::move(c)},
          m_capacity{capacity},
          m_eviction_margin{eviction_margin},
          m_task{"disk_cache eviction", ioc} {
        m_task.spawn([this]() { return eviction_task(); });
    }

    coro<void> eviction_task() {
        auto state = co_await boost::asio::this_coro::cancellation_state;
        boost::asio::steady_timer timer(
            co_await boost::asio::this_coro::executor);
        while (state.cancelled() == boost::asio::cancellation_type::none) {

            // TODO: make eviction interval configurable
            timer.expires_after(std::chrono::seconds(10));
            co_await timer.async_wait(boost::asio::use_awaitable);

            // TODO: control eviction size with setting get's parameter
            auto evicted = m_deletion_queue.pop(10 * MEBI_BYTE);
            if (!evicted.empty()) {
                auto addr = gather_address(evicted);
                try {
                    co_await utils::erase(m_storage, addr);
                    m_current_size.fetch_sub(addr.data_size(),
                                             std::memory_order_acq_rel);
                } catch (const std::exception& e) {
                    LOG_ERROR()
                        << "Failed to delete evicted objects: " << e.what();
                }
            }
        }
    }

    address
    gather_address(const std::vector<std::shared_ptr<object_handle>>& evicted) {
        address ret;
        for (auto& e : evicted) {
            ret.append(e->get_address());
        }
        return ret;
    }
};

} // namespace uh::cluster::proxy::cache::disk
