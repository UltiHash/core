#pragma once

#include "offset.h"

#include <common/etcd/candidate.h>
#include <common/etcd/utils.h>
#include <common/telemetry/log.h>
#include <common/utils/strings.h>
#include <functional>

namespace uh::cluster::storage {

struct storage_campaign_strategy : public campaign_strategy {
    storage_campaign_strategy(etcd_manager& etcd, std::size_t group_id,
                              std::size_t storage_id,
                              std::function<void()> callback)
        : m_etcd{etcd},
          m_group_id{group_id},
          m_storage_id{storage_id},
          m_callback{std::move(callback)},
          m_offset_publisher{m_etcd, m_group_id, m_storage_id} {}

    void pre_campaign() override {
        // TODO: get offset from this service
        std::size_t current_offset = 0;
        m_offset_publisher.put(current_offset);
    }

    void on_elected() override { m_callback(); }

    void post_campaign() override {}

private:
    etcd_manager& m_etcd;
    std::size_t m_group_id;
    std::size_t m_storage_id;
    std::function<void()> m_callback;
    offset_publisher m_offset_publisher;
};

} // namespace uh::cluster::storage
