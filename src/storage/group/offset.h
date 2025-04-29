#pragma once

#include "impl/prefix.h"

#include <common/etcd/subscriber/subscriber.h>
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
                      std::size_t num_storages, callback_t callback = nullptr)
        : m_prefix{get_storage_offset_prefix(group_id)},
          m_offsets{m_prefix.storage_hostports, num_storages},
          m_subscriber{etcd, m_prefix, {m_offsets}, std::move(callback)} {}
    auto get() { return m_offsets.get(); };

private:
    prefix_t m_prefix;
    vector_observer<std::size_t> m_offsets;
    subscriber m_subscriber;
};

} // namespace uh::cluster::storage
