#pragma once

#include "subscriber_observer.h"

#include <common/utils/strings.h>

namespace uh::cluster {

template <typename T> class sync_vector_observer : public subscriber_observer {
public:
    using callback_t = std::function<void(std::size_t index, T& value)>;
    sync_vector_observer(std::string expected_parent_key,
                         std::size_t num_members, T default_value = {},
                         callback_t callback = nullptr)
        : m_expected_parent_key{std::move(expected_parent_key)},
          m_default_value{std::move(default_value)},
          m_callback{std::move(callback)},
          m_values(num_members, m_default_value) {}

    sync_vector_observer(std::string expected_parent_key,
                         std::size_t num_members, callback_t callback)
        : sync_vector_observer(expected_parent_key, num_members, {},
                               std::move(callback)) {}

    void set(std::size_t id, const T& value) { m_values[id] = value; }

    /*
     * getters
     */
    const T& get(std::size_t id) const { return m_values[id]; }

    T& get(std::size_t id) { return m_values[id]; }

    const std::vector<T>& get() const { return m_values; }

    std::vector<T>& get() { return m_values; }

    /*
     * listener
     */
    bool on_watch(etcd_manager::response resp) {
        auto key = std::filesystem::path(resp.key);

        if (key.parent_path() != m_expected_parent_key)
            return false;

        auto id = stoul(key.filename().string());

        switch (get_etcd_action_enum(resp.action)) {
        case etcd_action::GET:
        case etcd_action::CREATE:
        case etcd_action::SET:
            m_values[id] = deserialize<T>(resp.value);
            break;
        case etcd_action::DELETE:
            m_values[id] = m_default_value;
            break;
        default:
            return false;
        }

        if (m_callback)
            m_callback(id, m_values[id]);

        return true;
    }

private:
    std::string m_expected_parent_key;
    T m_default_value;
    callback_t m_callback;
    std::vector<T> m_values;
};

} // namespace uh::cluster
