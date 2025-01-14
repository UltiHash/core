
#ifndef UH_CLUSTER_MONITOR_H
#define UH_CLUSTER_MONITOR_H

#include "common/telemetry/log.h"
#include <atomic>
#include <chrono>
#include <functional>
#include <map>
#include <sstream>
#include <thread>

namespace uh::cluster {
struct monitor {

    class monitor_scope {
        std::map<std::string,
                 std::function<void(std::stringstream&, const std::string&)>>::
            const_iterator m_itr;
        monitor_scope(const auto& itr, monitor& mon)
            : m_itr(itr),
              m(mon) {}
        explicit monitor_scope(monitor& mon)
            : m(mon) {}
        ~monitor_scope() {
            if (m.m_enabled) {
                std::lock_guard<std::mutex> lock(m.m_mutex);
                m.m_recorders.erase(m_itr);
            }
        }
        friend monitor;
        monitor& m;
    };

    void add_global(const std::string& name, const auto& val) {

        static auto& initialised = init();
        if (!initialised)
            return;

        std::lock_guard<std::mutex> lock(m_mutex);
        m_recorders.emplace(
            name, [&val](std::stringstream& stream, const auto& name) {
                stream << name << ":\t" << val;
            });
    }

    monitor_scope add_scoped(const std::string& name, const auto& val) {

        static auto& initialised = init();
        if (!initialised)
            return monitor_scope{*this};

        std::lock_guard<std::mutex> lock(m_mutex);
        auto itr = m_recorders.emplace(
            name, [&val](std::stringstream& stream, const auto& name) {
                stream << name << ":\t" << val;
            });
        return monitor_scope{itr, *this};
    }

    void add_fn(const std::string& name, const auto fn) {

        static auto& initialised = init();
        if (!initialised)
            return;
        std::lock_guard<std::mutex> lock(m_mutex);
        m_recorders.emplace(name,
                            [fn](std::stringstream& stream, const auto& name) {
                                stream << name << ":\t" << fn();
                            });
    }

    void stop() { m_stop = true; }

    static monitor& get() {
        static auto m = monitor();
        return m;
    }

    monitor(const monitor&) = delete;
    monitor(monitor&&) = delete;

private:
    monitor() = default;

    bool& init() {
        if (!m_enabled)
            return m_init;

        LOG_INFO() << "initializing monitor";

        m_watcher = std::thread([this] {
            while (!m_stop) {
                std::stringstream stream;
                stream << "monitoring data:\n";

                std::unique_lock<std::mutex> lock(m_mutex);
                for (const auto& recorder : m_recorders) {
                    recorder.second(stream, recorder.first);
                    stream << "\n";
                }
                lock.unlock();

                LOG_INFO() << stream.str();
                std::this_thread::sleep_for(
                    std::chrono::seconds(m_interval_secs));
            }
        });
        m_watcher.detach();
        m_init = true;
        return m_init;
    }

    std::map<std::string,
             std::function<void(std::stringstream&, const std::string&)>>
        m_recorders;
    std::atomic_bool m_stop = false;
    bool m_init = false;
    const bool m_enabled = true;
    const int m_interval_secs = 1;
    std::thread m_watcher;
    std::mutex m_mutex;
};
} // namespace uh::cluster
#endif // UH_CLUSTER_MONITOR_H
