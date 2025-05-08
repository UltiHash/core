#pragma once

#include "impl/prefix.h"

#include <common/etcd/subscriber.h>
#include <common/etcd/utils.h>
#include <common/utils/strings.h>

namespace uh::cluster::storage {

/*
 * Storage-wise publisher
 */
class offset_publisher {
public:
    offset_publisher(etcd_manager& etcd, std::size_t group_id,
                     std::size_t storage_id)
        : m_etcd{etcd},
          m_prefix{get_storage_offset_prefix(group_id)},
          m_storage_id{storage_id} {}

    ~offset_publisher() { m_etcd.rm(m_prefix); }

    void put(std::size_t val) {
        m_etcd.put(m_prefix[m_storage_id], serialize(val));
    }

private:
    etcd_manager& m_etcd;
    offset_prefix_t m_prefix;
    std::size_t m_storage_id;
};

/*
 * Group-wise subscriber
 */
class offset_subscriber {
public:
    using callback_t = subscriber::callback_t;
    offset_subscriber(etcd_manager& etcd, std::size_t group_id,
                      std::size_t num_storages)
        : m_prefix{get_storage_offset_prefix(group_id)},
          future{promise.get_future()},
          m_offsets{m_prefix.storage_hostports, num_storages, -1},
          m_subscriber{
              "offset_subscriber", etcd, m_prefix, {m_offsets}, [this]() {
                  this->callback();
              }} {}
    auto get() { return m_offsets.get(); };

    auto wait_and_get(std::chrono::seconds timeout = 5s) {
        future.wait_for(timeout);
        return get();
    }

private:
    void callback() {
        auto offsets = m_offsets.get();
        auto all_set = std::ranges::all_of(
            offsets, [](const auto& offset) { return *offset != -1; });
        if (all_set) {
            promise.set_value();
        }
    }

    prefix_t m_prefix;
    std::promise<void> promise;
    std::future<void> future;
    vector_observer<int> m_offsets;
    subscriber m_subscriber;
};

} // namespace uh::cluster::storage
