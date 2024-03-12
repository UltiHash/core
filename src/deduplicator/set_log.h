#ifndef UH_CLUSTER_SET_LOG_H
#define UH_CLUSTER_SET_LOG_H

#include "common/types/common_types.h"
#include <common/global_data/global_data_view.h>
#include <cstring>
#include <fcntl.h>
#include <filesystem>

namespace uh::cluster {

enum set_operation : char {
    INSERT,
    REMOVE,
};

class set_log {

    std::filesystem::path m_log_path;
    int m_log_file;

    struct entry {
        set_operation op{};
        uint128_t pointer;
        uint16_t size{};
        uint128_t prefix{0};
    };

public:
    explicit set_log(std::filesystem::path log_path)
        : m_log_path(std::move(log_path)),
          m_log_file(get_log_file(m_log_path)) {
        if (m_log_file <= 0) {
            perror(m_log_path.c_str());
            throw std::runtime_error("Could not open the transaction log file");
        }
        lseek(m_log_file, 0, SEEK_END);
    }

    void append(const entry& e) {

        char buf[sizeof(entry)];
        serialize(e, buf);

        if (sizeof buf != ::write(m_log_file, buf, sizeof buf)) [[unlikely]] {
            throw std::runtime_error("Could not write into the set log file");
        }
    }

    template <typename Set> void replay(Set&& set, global_data_view& storage) {

        const auto file_size = std::filesystem::file_size(m_log_path);
        size_t offset = 0;
        if (0 != lseek(m_log_file, 0, SEEK_SET)) [[unlikely]] {
            throw std::runtime_error("Could not seek the set log file");
        }
        while (offset < file_size) {
            auto op_fe = deserialize();
            switch (op_fe.first) {
            case set_operation::INSERT:
                set.emplace(op_fe.second.pointer, op_fe.second.size,
                            op_fe.second.prefix, storage);
                break;
            case set_operation::REMOVE:
                // set.erase({op_fe.second.pointer, op_fe.second.size,
                // op_fe.second.prefix, storage});
                break;
            default:
                throw std::invalid_argument("Invalid set log entry!");
            }
            offset += sizeof(entry);
        }

        recreate(set);
    }

    ~set_log() {
        fsync(m_log_file);
        close(m_log_file);
    }

private:
    static int get_log_file(const std::filesystem::path& path) {
        std::filesystem::create_directories(path.parent_path());
        if (std::filesystem::exists(path)) {
            return ::open(path.c_str(), O_RDWR | O_APPEND);
        } else {
            return ::open(path.c_str(), O_RDWR | O_APPEND | O_CREAT,
                          S_IWUSR | S_IRUSR);
        }
    }

    static void serialize(const auto& f, char* buf) {
        buf[0] = set_operation::INSERT;
        size_t offset = sizeof set_operation::INSERT;
        std::memcpy(buf + offset, f.pointer.get_data(), sizeof f.pointer);
        offset += sizeof f.pointer;
        std::memcpy(buf + offset, &f.size, sizeof f.size);
        offset += sizeof f.size;
        std::memcpy(buf + offset, f.prefix.get_data(), sizeof f.prefix);
    }

    [[nodiscard]] std::pair<set_operation, entry> deserialize() const {
        char buf[sizeof(entry)];
        if (const auto rc = ::read(m_log_file, buf, sizeof buf);
            rc != sizeof buf) [[unlikely]] {
            perror("Read error");
            throw std::runtime_error(
                "Could not read the set element from the set log file");
        }
        entry f;
        size_t offset = sizeof set_operation::INSERT;
        std::memcpy(f.pointer.ref_data(), buf + offset, sizeof f.pointer);
        offset += sizeof f.pointer;
        std::memcpy(&f.size, buf + offset, sizeof f.size);
        offset += sizeof f.size;
        std::memcpy(f.prefix.ref_data(), buf + offset, sizeof f.prefix);

        return {static_cast<set_operation>(buf[0]), f};
    }

    template <typename Set> void recreate(Set&& set) {
        const auto new_file_path =
            m_log_path.parent_path() / "_set_logger_tmp_file_new";
        const auto old_file_tmp_path =
            m_log_path.parent_path() / "_set_logger_tmp_file_original";
        try {

            const auto tmp_file = get_log_file(new_file_path);
            for (const auto& f : set) {

                char buf[sizeof(entry)];
                serialize(f, buf);

                if (sizeof buf != ::write(tmp_file, buf, sizeof buf))
                    [[unlikely]] {
                    throw std::runtime_error(
                        "Could not write into the log file");
                }
            }
            close(tmp_file);
            close(m_log_file);

            std::filesystem::rename(m_log_path, old_file_tmp_path);
            std::filesystem::rename(new_file_path, m_log_path);
            m_log_file = get_log_file(m_log_path);
        } catch (std::exception& e) {
            if (!std::filesystem::exists(m_log_path) and
                std::filesystem::exists(old_file_tmp_path)) {
                std::filesystem::rename(old_file_tmp_path, m_log_path);
                m_log_file = get_log_file(m_log_path);
            }
        }

        ::lseek(m_log_file, 0, SEEK_END);
        std::filesystem::remove(old_file_tmp_path);
    }
};
} // end namespace uh::cluster

#endif // UH_CLUSTER_SET_LOG_H
