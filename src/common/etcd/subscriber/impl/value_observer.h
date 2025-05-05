#pragma once

#include "subscriber_observer.h"

#include <common/utils/strings.h>

namespace uh::cluster {

template <typename T> class value_observer : public subscriber_observer {
public:
    using callback_t = std::function<void(T&)>;
    value_observer(std::string expected_key, T default_value = {},
                   callback_t callback = nullptr)
        : m_expected_key{std::move(expected_key)},
          m_default_value{std::move(default_value)},
          m_callback{std::move(callback)} {

        m_value.store(std::make_shared<T>(m_default_value),
                      std::memory_order_release);
    }

    /*
     * getters
     */
    std::shared_ptr<T> get() const {
        return m_value.load(std::memory_order_acquire);
    }

    /*
     * listener
     */
    bool on_watch(etcd_manager::response resp) {

        if (resp.key != m_expected_key)
            return false;

        switch (get_etcd_action_enum(resp.action)) {
        case etcd_action::GET:
        case etcd_action::CREATE:
        case etcd_action::SET:
            m_value.store(std::make_shared<T>(deserialize<T>(resp.value)),
                          std::memory_order_release);
            break;
        case etcd_action::DELETE:
            m_value.store(std::make_shared<T>(m_default_value),
                          std::memory_order_release);
            break;
        default:
            return false;
        }

        if (m_callback)
            m_callback(*get().get());

        return true;
    }

private:
    std::string m_expected_key;
    T m_default_value;
    callback_t m_callback;
    std::atomic<std::shared_ptr<T>> m_value;
};

} // namespace uh::cluster
