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
    using cache_interface_t = cache_interface<object_metadata, object_handle>;
    using lru_cache_t = lru_cache<object_metadata, object_handle>;
    using lfu_cache_t = lfu_cache<object_metadata, object_handle>;
    using deletion_queue_t = deletion_queue<object_metadata, object_handle>;

    using data_view = storage::data_view;
    using stream = ep::http::stream;
    using body = ep::http::body;

    /*
     * Store object handle in cache
     *
     * It removed address information from the given body.
     */
    coro<void> put(object_metadata key, reader_body& body) {
        auto objh = body.get_object_handle();
        auto obj_size = objh.data_size();

        auto total_size =
            m_current_size.load(std::memory_order_acquire) + obj_size;
        auto threshold = m_capacity;

        auto required_size =
            total_size > threshold ? total_size - threshold : 0;
        if (required_size > 0) {
            auto evicted = m_cache->evict(required_size);
            auto addr = gather_address(evicted);
            co_await m_storage.unlink(addr);

            // apply size change "after ERASE", "before PUT"
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
        return manager(ioc, storage, std::make_unique<lru_cache_t>(), capacity,
                       eviction_margin);
    }

private:
    data_view& m_storage;
    std::unique_ptr<cache_interface_t> m_cache;
    std::size_t m_capacity;
    std::atomic<std::size_t> m_current_size{0};

    deletion_queue_t m_deletion_queue;
    // TODO: spawn a background task to remove
    scoped_task m_task;

    manager(boost::asio::io_context& ioc, data_view& storage,
            std::unique_ptr<cache_interface_t> c, std::size_t capacity,
            std::size_t eviction_margin)
        : m_storage{storage},
          m_cache{std::move(c)},
          m_capacity{capacity},
          m_task{"disk_cache eviction", ioc, eviction_task()} {}

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
                    co_await m_storage.unlink(addr);
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
