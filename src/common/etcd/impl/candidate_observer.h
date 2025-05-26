#pragma once

#include "subscriber_observer.h"

#include <common/utils/strings.h>

namespace uh::cluster {

class candidate_observer : public subscriber_observer {
public:
    using id_t = int; // to represent -1 as empty
    constexpr static id_t staging_id = -2;
    constexpr static id_t default_id = -1;
    using callback_t = std::function<void(bool is_leader)>;
    candidate_observer(etcd_manager& etcd, std::string expected_key, id_t id,
                       callback_t after_campaign = nullptr,
                       callback_t after_proclaim = nullptr)
        : m_etcd{etcd},
          m_expected_key{std::move(expected_key)},
          m_id{id},
          m_after_campaign{std::move(after_campaign)},
          m_after_proclaim{std::move(after_proclaim)} {

        m_is_leader.store(campaign(), std::memory_order_release);
    }

    ~candidate_observer() {
        if (is_leader()) {
            m_etcd.rm(m_expected_key);
        }
    }

    /*
     * getters
     */
    auto is_leader() const -> bool {
        return m_is_leader.load(std::memory_order_acquire);
    }

    /*
     * listener
     */
    bool on_watch(etcd_manager::response resp) {

        if (resp.key != m_expected_key)
            return false;

        switch (get_etcd_action_enum(resp.action)) {
        case etcd_action::SET: {
            auto val = deserialize<id_t>(resp.value);
            if (val == default_id) {
                throw std::runtime_error(
                    "Setting candidate key to default_id is not allowed");
            }
            if (val != staging_id) {
                if (m_after_proclaim) {
                    m_after_proclaim(is_leader());
                }
            }
            break;
        }
        case etcd_action::DELETE:
            m_is_leader.store(campaign(), std::memory_order_release);
            break;
        default:
            return false;
        }

        return true;
    }

private:
    /*
     * @return true if it wins the campaign
     */
    auto campaign() -> bool {

        // Create key with -1 first,
        auto resp =
            m_etcd.create_if_empty(m_expected_key, serialize<int>(staging_id));

        if (m_after_campaign) {
            m_after_campaign(resp.is_ok());
        }

        m_etcd.put(m_expected_key, serialize<int>(m_id));

        auto won = resp.is_ok();
        return won;
    }

    etcd_manager& m_etcd;
    std::string m_expected_key;
    id_t m_id;
    callback_t m_after_campaign;
    callback_t m_after_proclaim;
    std::atomic<bool> m_is_leader{false};
};

} // namespace uh::cluster
