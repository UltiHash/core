#pragma once

#include "subscriber_observer.h"

namespace uh::cluster {

template <Serializable T>
class sync_value_observer : public subscriber_observer {
public:
    using callback_t = std::function<void(T& value)>;
    sync_value_observer(std::string expected_key, T default_value = {},
                        callback_t callback = nullptr)
        : m_expected_key{std::move(expected_key)},
          m_default_value{std::move(default_value)},
          m_callback{std::move(callback)},
          m_value{m_default_value} {}

    sync_value_observer(std::string expected_key, callback_t callback)
        : sync_value_observer(expected_key, {}, std::move(callback)) {}

    void set(const T& value) { m_value = value; }

    /*
     * getter
     */
    const T& get() const { return m_value; }

    T& get() { return m_value; }

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
            m_value = deserialize<T>(resp.value);
            break;
        case etcd_action::DELETE:
            m_value = m_default_value;
            break;
        default:
            return false;
        }

        if (m_callback)
            m_callback(m_value);

        return true;
    }

private:
    std::string m_expected_key;
    T m_default_value;
    callback_t m_callback;
    T m_value;
};

} // namespace uh::cluster
