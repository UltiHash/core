
#ifndef UH_CLUSTER_DEDUPE_LOGGER_H
#define UH_CLUSTER_DEDUPE_LOGGER_H

#include <config.h>

#include "common/types/address.h"
#include "common/utils/strings.h"
#include <filesystem>
#include <fstream>
#include <sstream>

namespace uh::cluster {
struct dedupe_logger {

    dedupe_logger(const std::filesystem::path& log_path, size_t chunk_size)
        : m_chunk_size(chunk_size) {
        if constexpr (enable_log) {
            m_log_file =
                std::fstream(log_path, std::fstream::out | std::fstream::app);
            if (!m_log_file) {
                throw std::runtime_error("Could not open the set log file");
            }
        }
        m_log.reserve(m_chunk_size);
    }

    template <typename... T> inline void log_deduplication(T&&... t) {
        if constexpr (enable_log) {
            log_entry("dedupe", std::forward<T>(t)...);
        }
    }

    template <typename... T> inline void log_pursue_deduplication(T&&... t) {
        if constexpr (enable_log) {
            log_entry("pursue", std::forward<T>(t)...);
        }
    }

    template <typename... T> inline void log_non_deduplication(T&&... t) {
        if constexpr (enable_log) {
            log_entry("non-dedupe", std::forward<T>(t)...);
        }
    }

    template <typename... T> inline void log_stat(T&&... t) {
        if constexpr (enable_log) {
            log_entry("stat", std::forward<T>(t)...);
        }
    }

    ~dedupe_logger() {
        for (const auto& entry : m_log) {
            m_log_file << entry << "\n";
        }
    }

private:
    template <typename... T>
    void log_entry(const std::string& log_type, T&&... t) {
        auto log_entry = log_type;
        ((log_entry += " " + get_string(std::forward<T>(t))), ...);

        std::lock_guard<std::mutex> lock(m_mutex);
        m_log.emplace_back(std::move(log_entry));

        if (m_log.size() == m_log.capacity()) {
            for (const auto& entry : m_log) {
                m_log_file << entry << "\n";
            }
            m_log.clear();
        }
    }

    static inline std::string get_string(size_t num) noexcept {
        return std::to_string(num);
    }
    static inline std::string get_string(uint128_t num) noexcept {
        return std::to_string(num.get_low());
    }
    static inline std::string get_string(const std::string& val) noexcept {
        return to_hex(val);
    }
    static inline std::string get_string(const fragment& f) noexcept {
        return get_string(f.pointer) + " " + get_string(f.size);
    }

#ifdef LOG_DEDUPE_SET
    static constexpr bool enable_log = true;
#else
    static constexpr bool enable_log = false;
#endif
    const size_t m_chunk_size;
    std::fstream m_log_file;
    std::vector<std::string> m_log;
    std::mutex m_mutex;
};
} // namespace uh::cluster

#endif // UH_CLUSTER_DEDUPE_LOGGER_H
