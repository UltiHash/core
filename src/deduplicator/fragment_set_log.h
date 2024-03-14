#ifndef UH_CLUSTER_FRAGMENT_SET_LOG_H
#define UH_CLUSTER_FRAGMENT_SET_LOG_H

#include "common/global_data/global_data_view.h"
#include "common/types/common_types.h"
#include "fragment_set_element.h"

#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <mutex>

namespace uh::cluster {

enum set_operation : char {
    INSERT,
};

class fragment_set_log {

    std::filesystem::path m_log_path;
    int m_log_file;

    struct log_entry {
        set_operation op{};
        uint128_t pointer;
        uint16_t size{};
        uint128_t prefix{0};
    };

public:
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

private:
    static int get_log_file(const std::filesystem::path& path);
    static void serialize_entry(const log_entry& entry, char* buf);
    static log_entry deserialize_entry(const char* buf);
    [[nodiscard]] log_entry read_entry();
};

} // end namespace uh::cluster

#endif // UH_CLUSTER_FRAGMENT_SET_LOG_H
