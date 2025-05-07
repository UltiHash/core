#pragma once

#include <common/etcd/impl/subscriber_observer.h>

#include <common/etcd/utils.h>
#include <common/telemetry/log.h>

namespace uh::cluster {

class subscriber;

/*
 * reader manages multiple keys by using recursive watch.
 */
class reader {
public:
    using callback_t = std::function<void()>;

    reader(std::string name, etcd_manager& etcd, const std::string& key,
           std::initializer_list<std::reference_wrapper<subscriber_observer>>
               observers,
           callback_t callback = nullptr)
        : m_name{std::move(name)},
          m_etcd{etcd},
          m_observers{observers},
          m_callback{std::move(callback)} {

        auto change_detected = false;
        m_index = m_etcd.ls(
            key, [this, &change_detected](etcd_manager::response resp) {
                try {
                    change_detected = on_watch(resp);
                } catch (const std::exception& e) {
                    LOG_WARN() << "Exception on reader: " << e.what();
                }
            });

        if (change_detected)
            run_callback();
    }

    auto get_index() { return m_index; }

private:
    friend subscriber;

    void run_callback() {
        if (m_callback)
            m_callback();
    }

    bool on_watch(etcd_manager::response resp) {
        try {
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
    int64_t m_index;
};

} // namespace uh::cluster

#include <common/etcd/impl/hostports_observer.h>
#include <common/etcd/impl/value_observer.h>
#include <common/etcd/impl/vector_observer.h>
