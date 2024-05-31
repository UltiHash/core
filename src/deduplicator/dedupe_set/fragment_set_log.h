#ifndef UH_CLUSTER_FRAGMENT_SET_LOG_H
#define UH_CLUSTER_FRAGMENT_SET_LOG_H

#include "common/caches/plain_cache.h"
#include "common/types/common_types.h"
#include "deduplicator/config.h"
#include "fragment_set_element.h"

#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <set>

namespace uh::cluster {

enum set_operation : char {
    INSERT,
    REMOVE,
};

class fragment_set_log {

public:
    static constexpr std::size_t m_entry_size =
        sizeof(set_operation) + sizeof(uint16_t) + sizeof(uint128_t) +
        PREFIX_SIZE + sizeof(uint16_t);

    struct log_entry {
        set_operation op{};
        uint128_t pointer;
        uint16_t size{};
        uint16_t prefix_size{};
        char prefix[PREFIX_SIZE]{};

        auto operator<=>(const log_entry&) const = default;
        using serialize = zpp::bits::members<5>;

        log_entry() = default;
        log_entry(set_operation set_op, const uint128_t& f_pointer,
                  uint16_t f_size, const std::string& f_prefix)
            : op{set_op},
              pointer{f_pointer},
              size{f_size},
              prefix_size{static_cast<uint16_t>(f_prefix.size())} {
            memcpy(prefix, f_prefix.data(), prefix_size);
        }

        log_entry(set_operation set_op, const uint128_t& f_pointer,
                  uint16_t f_size = 0)
            : op{set_op},
              pointer{f_pointer},
              size{f_size} {}

        log_entry(const log_entry& le)
            : op{le.op},
              pointer{le.pointer},
              size{le.size},
              prefix_size{le.prefix_size} {
            std::memcpy(prefix, le.prefix, prefix_size);
        }
    };
    /**
     * @brief Constructs a fragment_set_log from a specified path.
     *
     * The fragment_set_log is responsible for logging all actions performed on
     * the owning fragment_set, so that the state of the fragment_set can be
     * restored after a restart of the application. If no log file exists under
     * the specified path, a new log file is created.
     *
     * @param log_path A std::filesystem::path to the log file to be
     * used/created.
     */
    explicit fragment_set_log(std::filesystem::path log_path);

    /**
     * @brief Destructs the fragment_set_log object, closes log file handle.
     *
     * The destructor calls fsync on the file handle and closes it.
     */
    ~fragment_set_log();

    /**
     * @brief Appends a log_entry to the log file.
     *
     * A log_entry is appended to the log file and fsync is called to
     * make sure the appended entry has been persisted.
     *
     * @param entry A constant reference the the log_entry to be appended to the
     * log file.
     */
    void append(const log_entry& entry);

    /**
     * @brief Restores the fragment_set from the log file upon startup.
     *
     * The replay method is used by the constructor of the fragment_set class
     * to restore the state of the set after a restart of the deduplicator
     * service.
     *
     * @param set A reference to a std::set<fragment_set_element> to be
     * restored.
     * @param storage A reference to a global_data_view instance.
     */
    void replay(std::set<fragment_set_element>& set, global_data_view& storage);

    /**
     * @brief synchronizes the log file with the underlying storage device
     *
     * Calls std::fstream::flush() on log file to synchronize cached writes
     * to persistent storage.
     */
    void flush();

private:
    [[nodiscard]] log_entry read_entry();

    std::filesystem::path m_log_path;
    std::fstream m_log_file;
    std::mutex m_mutex;
    plain_cache m_cache;
};

} // end namespace uh::cluster

#endif // UH_CLUSTER_FRAGMENT_SET_LOG_H
