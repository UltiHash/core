#pragma once

#include <common/etcd/subscriber/impl/subscriber_observer.h>

#include <common/etcd/utils.h>
#include <common/telemetry/log.h>

namespace uh::cluster {

/*
 * Group-wise subscriber
 */
// TODO: use const key_t& instead of using T
template <typename T> class subscriber {
public:
    using callback_t = std::function<void(etcd_manager::response resp)>;

    subscriber(
        etcd_manager& etcd, const T& key,
        std::initializer_list<std::reference_wrapper<subscriber_observer>>
            observers,
        callback_t callback = nullptr)
        : m_etcd{etcd},
          m_observers{observers},
          m_callback{std::move(callback)} {

        m_wg = m_etcd.watch(
            key, [this](etcd_manager::response resp) { on_watch(resp); });
    }

private:
    void on_watch(etcd_manager::response resp) {
        try {
            LOG_INFO() << std::format(
                "subscriber has detected {} action on {} with value {}",
                resp.action, resp.key, resp.value);

            bool change_detected =
                std::any_of(m_observers.begin(), m_observers.end(),
                            [&](auto& o) { return o.get().on_watch(resp); });

            if (change_detected and m_callback)
                m_callback(resp);

        } catch (const std::runtime_error& e) {
            LOG_WARN() << "if you see stoul exception, it might be the case "
                          "deserialize function get's wrong value: "
                       << e.what();

        } catch (const std::exception& e) {
            LOG_WARN() << "error updating externals: " << e.what();
        }
    }

    etcd_manager& m_etcd;
    std::vector<std::reference_wrapper<subscriber_observer>> m_observers;
    callback_t m_callback;
    etcd_manager::watch_guard m_wg;
};

} // namespace uh::cluster

#include <common/etcd/subscriber/impl/value_observer.h>
#include <common/etcd/subscriber/impl/vector_observer.h>
