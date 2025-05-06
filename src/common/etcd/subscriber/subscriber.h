#pragma once

#include <common/etcd/subscriber/impl/subscriber_observer.h>

#include <common/etcd/utils.h>
#include <common/telemetry/log.h>

namespace uh::cluster {

/*
 * Subscriber manages multiple keys by using recursive watch.
 */
class subscriber {
public:
    using callback_t = std::function<void()>;

    subscriber(
        std::string name, etcd_manager& etcd, const std::string& key,
        std::initializer_list<std::reference_wrapper<subscriber_observer>>
            observers,
        callback_t callback = nullptr)
        : m_name{std::move(name)},
          m_etcd{etcd},
          m_observers{observers},
          m_callback{std::move(callback)} {

        auto change_detected = false;
        auto index = m_etcd.ls(
            key, [this, &change_detected](etcd_manager::response resp) {
                try {
                    change_detected = on_watch(resp);
                } catch (const std::exception& e) {
                    LOG_WARN() << "Exception on subscriber: " << e.what();
                }
            });

        if (change_detected and m_callback)
            m_callback();

        m_wg = m_etcd.watch(
            key,
            [this](etcd_manager::response resp) {
                try {
                    auto change_detected = on_watch(resp);
                    if (change_detected and m_callback)
                        m_callback();
                } catch (const std::exception& e) {
                    LOG_WARN() << "Exception on subscriber: " << e.what();
                }
            },
            index + 1);
    }

private:
    bool on_watch(etcd_manager::response resp) {
        try {
            LOG_INFO() << std::format(
                "subscriber {} has detected {} action on {} with value {}",
                m_name, resp.action, resp.key, resp.value);

            bool change_detected =
                std::any_of(m_observers.begin(), m_observers.end(),
                            [&](auto& o) { return o.get().on_watch(resp); });

            return change_detected;

        } catch (const std::runtime_error& e) {
            LOG_WARN() << "if you see stoul exception, it might be the case "
                          "deserialize function get's wrong value: "
                       << e.what();
            return false;

        } catch (const std::exception& e) {
            LOG_WARN() << "error updating externals: " << e.what();
            return false;
        }
        return false;
    }

    std::string m_name;
    etcd_manager& m_etcd;
    std::vector<std::reference_wrapper<subscriber_observer>> m_observers;
    callback_t m_callback;
    etcd_manager::watch_guard m_wg;
};

} // namespace uh::cluster

#include <common/etcd/subscriber/impl/hostports_observer.h>
#include <common/etcd/subscriber/impl/value_observer.h>
#include <common/etcd/subscriber/impl/vector_observer.h>
