#pragma once

#include <common/etcd/endpoints/impl/subscriber_observer.h>

#include <common/etcd/utils.h>
#include <common/telemetry/log.h>

namespace uh::cluster {

/*
 * Group-wise subscriber
 */
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
                "externals_watcher has detected {} action on {} with value {}",
                resp.action, resp.key, resp.value);

            for (auto& o : m_observers) {
                o.get().on_watch(resp);
            }

            if (m_callback)
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

#include <common/etcd/endpoints/impl/value_observer.h>
#include <common/etcd/endpoints/impl/vector_observer.h>
