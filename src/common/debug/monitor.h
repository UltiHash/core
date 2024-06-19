
#ifndef UH_CLUSTER_MONITOR_H
#define UH_CLUSTER_MONITOR_H

#include <map>
#include <sstream>
#include <functional>
#include <atomic>
#include <thread>
#include "common/telemetry/log.h"

namespace uh::cluster {
struct monitor {

    class monitor_scope {
        std::map<std::string, std::function<void(std::stringstream&, const std::string&)>>::const_iterator m_itr;
        explicit monitor_scope(const auto& itr): m_itr(itr) {}
        monitor_scope() = default;
        ~monitor_scope() {
            if (m_enabled) {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_recorders.erase(m_itr);
            }
        }
        friend monitor;
    };
    static void add_global(const std::string& name, const auto& val) {
        if (!m_enabled)
            return;

        static auto running = init();
        if (!running)
            return;

        std::lock_guard<std::mutex> lock (m_mutex);
        m_recorders.emplace(name, [&val](std::stringstream& stream, const auto& name) {
            stream << name << ":\t" << val;
        });
    }

    static monitor_scope add_scoped(const std::string& name, const auto& val) {
        if (!m_enabled)
            return {};

        static auto running = init();
        if (!running)
            return {};

        std::lock_guard<std::mutex> lock (m_mutex);
        auto itr = m_recorders.emplace(name, [&val](std::stringstream& stream, const auto& name) {
            stream << name << ":\t" << val;
        });
        return itr;
    }


    static void stop () {
        m_stop = true;
    }

private:

    static bool init() {
        LOG_INFO() << "initializing monitor";

        m_watcher = std::thread([] {
            while (!m_stop) {
                std::stringstream stream;
                stream << "monitoring data:\n";

                std::unique_lock<std::mutex> lock (m_mutex);
                for (const auto& recorder : m_recorders) {
                    recorder.second(stream, recorder.first);
                    stream << "\n";
                }
                lock.unlock();

                LOG_INFO() << stream.str();
                sleep (m_interval_secs);
            }
        });
        m_watcher.detach();
        return true;
    }

    inline static std::map<std::string, std::function<void(std::stringstream&, const std::string&)>>
        m_recorders;
    inline static std::atomic_bool m_stop = false;
    inline static const bool m_enabled = true;
    inline static const int m_interval_secs = 1;
    inline static std::thread m_watcher;
    inline static std::mutex m_mutex;
};
}
#endif // UH_CLUSTER_MONITOR_H
