//
// Created by masi on 10/2/23.
//

#ifndef CORE_TRANSACTION_LOG_H
#define CORE_TRANSACTION_LOG_H


#include "unordered_map"
#include "common/utils/common.h"
#include <string>
#include <list>
#include <fstream>
#include <filesystem>
#include <fcntl.h>

namespace uh::cluster {
    class transaction_log {

        std::filesystem::path m_log_path;
        int m_log_file;

    public:
        enum operation:char {
            INSERT_START = 'i',
            INSERT_END = 'I',
            REMOVE_START = 'r',
            REMOVE_END = 'R',
            UPDATE_START = 'u',
            UPDATE_END = 'U',
            INSERT_ = 'a',
        };

        explicit transaction_log (std::filesystem::path log_path):
            m_log_path (std::move (log_path)),
            m_log_file (get_log_file (m_log_path)) {
            if (m_log_file <= 0) {
                perror (m_log_path.c_str());
                throw std::runtime_error ("Could not open the transaction log file");
            }
            lseek(m_log_file, 0, SEEK_END);
        }

        void append (std::string_view key, uint64_t object_id, operation op) const {
            const auto buf = serialise({.key = key, .op = op, .object_id = object_id});
            size_t total_size = 0;
            while (total_size < buf.size()) {
                const auto ws = ::write (m_log_file, buf.data(), buf.size());
                if (ws < 0) [[unlikely]] {
                    throw std::runtime_error ("Could not write into the log file");
                }
                total_size += ws;
            }
        }

        std::map <std::string, uint64_t> replay () {

            std::map <std::string, uint64_t> log_map;
            std::unordered_map <std::string, uint64_t> dangling_inserts;
            std::unordered_map <std::string, uint64_t> dangling_deletes;
            std::unordered_map <std::string, uint64_t> dangling_updates;
            const auto file_size = std::filesystem::file_size (m_log_path);
            size_t offset = 0;
            if (0 != lseek(m_log_file, 0, SEEK_SET)) [[unlikely]] {
                throw std::runtime_error ("Could not seek the log file");
            }
            while (offset < file_size)
            {
                auto e = deserialize(m_log_file);
                const auto key_size = std::get <std::string> (e.key).size();
                switch (e.op) {
                    case operation::INSERT_:
                        log_map.emplace(std::move (std::get <std::string> (e.key)), e.object_id);
                        break;
                    case operation::INSERT_END:
                        dangling_inserts.erase(std::get <std::string> (e.key));
                        log_map.emplace(std::move (std::get <std::string> (e.key)), e.object_id);
                        break;
                    case operation::UPDATE_END:
                        dangling_updates.erase (std::get <std::string> (e.key));
                        log_map.at (std::get <std::string> (e.key)) = e.object_id;
                        break;
                    case operation::REMOVE_END:
                        dangling_deletes.erase (std::get <std::string> (e.key));
                        log_map.erase(std::get <std::string> (e.key));
                        break;
                    case operation::INSERT_START:
                        dangling_inserts.emplace (std::move (std::get <std::string> (e.key)), e.object_id);
                        break;
                    case operation::UPDATE_START:
                        dangling_updates.emplace (std::move (std::get <std::string> (e.key)), e.object_id);
                        break;
                    case operation::REMOVE_START:
                        dangling_deletes.emplace (std::move (std::get <std::string> (e.key)), e.object_id);
                        break;
                    default:
                        throw std::invalid_argument ("Invalid log entry!");
                }

                offset += sizeof (uint16_t) + sizeof e.op + sizeof e.object_id + key_size;
            }

            if (!dangling_inserts.empty()) {
                throw std::runtime_error ("Existing dangling insertions!");
            }
            if (!dangling_updates.empty()) {
                throw std::runtime_error ("Existing dangling updates!");
            }
            if (!dangling_deletes.empty()) {
                throw std::runtime_error ("Existing dangling deletions!");
            }

            recreate (log_map);

            return log_map;
        }

        ~transaction_log() {
            close (m_log_file);
        }

    private:

        struct entry {
            std::variant <std::string, std::string_view> key;
            operation op;
            uint64_t object_id;
        };

        static unique_buffer <char> serialise (const entry& entry_) {
            const uint16_t key_size = std::get <std::string_view> (entry_.key).size();
            unique_buffer<char> buf (sizeof entry_.op + sizeof entry_.object_id + sizeof key_size + key_size);
            std::memcpy (buf.data(), &key_size, sizeof key_size);
            buf.data()[sizeof key_size] = entry_.op;
            std::memcpy (buf.data() + sizeof key_size + sizeof entry_.op, &entry_.object_id, sizeof entry_.object_id);
            std::memcpy (buf.data() + sizeof key_size + sizeof entry_.op + sizeof entry_.object_id, std::get <std::string_view> (entry_.key).data(), key_size);
            return buf;
        }

         static entry deserialize (const int fd) {
            uint16_t key_size;
            char buf [sizeof key_size + sizeof entry::op + sizeof entry::object_id];
            if (const auto rc = ::read(fd, buf, sizeof buf); rc != sizeof buf) [[unlikely]] {
                perror ("Read error");
                throw std::runtime_error ("Could not read the entry from the log file");
            }
            key_size = *reinterpret_cast <uint16_t*> (buf);
            entry e;
            e.key = std::string ();
            std::get <std::string> (e.key).resize(key_size);

            if (key_size != ::read (fd, std::get <std::string> (e.key).data(), key_size)) [[unlikely]] {
                throw std::runtime_error ("Could not read the entry from the log file");
            }
            e.op = static_cast <operation> (buf [sizeof key_size]);
            std::memcpy (&e.object_id, buf + sizeof key_size + sizeof e.op, sizeof e.object_id);
            return e;
        }

        void recreate (const std::map <std::string, uint64_t>& log_map) {
            const auto new_file_path = m_log_path.parent_path() / "_key_logger_tmp_file_new";
            const auto old_file_tmp_path = m_log_path.parent_path() / "_key_logger_tmp_file_original";
            try {

                const auto tmp_file = get_log_file(new_file_path);
                for (const auto &item: log_map) {
                    const auto buf = serialise({.key = std::string_view (item.first), .op = operation::INSERT_, .object_id = item.second});

                    size_t total_size = 0;
                    while (total_size < buf.size()) {
                        const auto ws = ::write (tmp_file, buf.data(), buf.size());
                        if (ws < 0) [[unlikely]] {
                            throw std::runtime_error ("Could not write into the log file");
                        }
                        total_size += ws;
                    }
                }
                close(tmp_file);
                close(m_log_file);

                std::filesystem::rename(m_log_path, old_file_tmp_path);
                std::filesystem::rename(new_file_path, m_log_path);
                m_log_file = get_log_file(m_log_path);
            }
            catch (std::exception& e) {
                if (!std::filesystem::exists(m_log_path) and std::filesystem::exists(old_file_tmp_path)) {
                    std::filesystem::rename (old_file_tmp_path, m_log_path);
                    m_log_file = get_log_file(m_log_path);
                }
            }

            ::lseek(m_log_file, 0, SEEK_END);
            std::filesystem::remove (old_file_tmp_path);
        }

        static int get_log_file (const std::filesystem::path& path) {
            if (std::filesystem::exists (path)) {
                return ::open (path.c_str(), O_RDWR | O_APPEND);
            }
            else {
                return ::open (path.c_str(), O_RDWR | O_APPEND | O_CREAT, S_IWUSR | S_IRUSR);
            }
        }
    };
} // end namespace uh::cluster


#endif //CORE_TRANSACTION_LOG_H
