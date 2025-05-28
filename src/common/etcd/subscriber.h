#pragma once

#include <common/etcd/impl/subscriber_observer.h>
#include <common/etcd/reader.h>

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
        : m_reader{name, etcd, key, observers, std::move(callback)} {

        m_wg = etcd.watch(
            key,
            [this](etcd_manager::response resp) { m_reader.on_watch(resp); },
            m_reader.get_index() + 1);
    }

private:
    reader m_reader;
    etcd_manager::watch_guard m_wg;
};

} // namespace uh::cluster
