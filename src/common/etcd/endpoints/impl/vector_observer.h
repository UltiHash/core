#pragma once

#include "subscriber_observer.h"

#include <common/utils/strings.h>

namespace uh::cluster {

template <typename T> class vector_observer : public subscriber_observer {
public:
    using callback_t = std::function<void(T&)>;
    vector_observer(std::string expected_parent_key, size_t num_storages,
                    callback_t callback = nullptr, T default_value = {})
        : m_expected_parent_key{std::move(expected_parent_key)},
          m_default_value{std::move(default_value)},
          m_callback{std::move(callback)},
          m_values(num_storages) {

        for (auto& atom : m_values) {
            atom.store(std::make_shared<T>(m_default_value),
                       std::memory_order_release);
        }
    }

    /*
     * getters
     */
    std::vector<std::shared_ptr<T>> get() const {
        std::vector<std::shared_ptr<T>> result;
        result.reserve(m_values.size());
        for (const auto& atom : m_values) {
            result.push_back(atom.load(std::memory_order_acquire));
        }
        return result;
    }

    /*
     * listener
     */
    void on_watch(etcd_manager::response resp) {

        auto key = std::filesystem::path(resp.key);

        if (key.parent_path() != m_expected_parent_key)
            return;

        auto id = stoul(key.filename().string());

        switch (get_etcd_action_enum(resp.action)) {
        case etcd_action::GET:
        case etcd_action::CREATE:
        case etcd_action::SET:
            m_values[id].store(std::make_shared<T>(deserialize<T>(resp.value)));
            break;
        case etcd_action::DELETE:
            m_values[id].store(std::make_shared<T>(m_default_value));
            break;
        default:
            break;
        }

        if (m_callback)
            m_callback(*get(id).get());
    }

private:
    std::string m_expected_parent_key;
    T m_default_value;
    callback_t m_callback;
    std::vector<std::atomic<std::shared_ptr<T>>> m_values;

    auto get(size_t storage_id) const {
        return m_values[storage_id].load(std::memory_order_acquire);
    }
};

} // namespace uh::cluster
