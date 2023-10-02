//
// Created by masi on 10/2/23.
//

#ifndef CORE_KEY_LOGGER_H
#define CORE_KEY_LOGGER_H


#include "unordered_map"
#include <common/common.h>
#include <string>
#include <list>
#include <fstream>
#include <filesystem>

namespace uh::cluster {
    class key_logger {

        std::filesystem::path m_log_path;
        std::fstream m_log_file;


    public:
        enum operation:char {
            INSERT_START = 'i',
            INSERT_END = 'I',
            DELETE_START = 'd',
            DELETE_END = 'D',
            UPDATE_START = 'u',
            UPDATE_END = 'U',
            INSERT = 'a',
        };

        explicit key_logger (std::filesystem::path log_path):
            m_log_path (std::move (log_path)),
            m_log_file (get_log_file (m_log_path)) {
            if (!m_log_file.is_open()) {
                perror (m_log_path.c_str());
                throw std::runtime_error ("Could not open the key logger file");
            }
            m_log_file.seekp(0, std::ios::end);
        }

        void append (std::string_view key, uint64_t object_id, operation op) {
            m_log_file << op << key << std::hex << std::setw(sizeof(object_id)) << object_id << std::endl;
            m_log_file.sync();
        }

        std::unordered_map <std::string, uint64_t> replay () {
            m_log_file.seekp(0, std::ios::beg);
            std::unordered_map <std::string, uint64_t> log_map;
            std::string line;
            std::unordered_map <std::string, uint64_t> dangling_inserts;
            std::unordered_map <std::string, uint64_t> dangling_deletes;
            std::unordered_map <std::string, uint64_t> dangling_updates;
            while (std::getline(m_log_file, line))
            {
                const auto op = static_cast <operation> (line[0]);
                const auto object_id = std::strtoul (line.substr(line.size() - sizeof (uint64_t)).c_str(), nullptr, 16);
                auto key = line.substr(1, line.size() - sizeof(object_id) - sizeof(op));
                switch (op) {
                    case operation::INSERT:
                        log_map.emplace(std::move (key), object_id);
                        break;
                    case operation::INSERT_END:
                        dangling_inserts.erase(key);
                        log_map.emplace(std::move (key), object_id);
                        break;
                    case operation::UPDATE_END:
                        dangling_updates.erase (key);
                        log_map.at (key) = object_id;
                        break;
                    case operation::DELETE_END:
                        dangling_deletes.erase (key);
                        log_map.erase(key);
                        break;
                    case operation::INSERT_START:
                        dangling_inserts.emplace (std::move (key), object_id);
                        break;
                    case operation::UPDATE_START:
                        dangling_updates.emplace (std::move (key), object_id);
                        break;
                    case operation::DELETE_START:
                        dangling_deletes.emplace (std::move (key), object_id);
                        break;
                    default:
                        throw std::invalid_argument ("Invalid log entry!");
                }
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

    private:

        void recreate (const std::unordered_map <std::string, uint64_t>& log_map) {
            const auto new_file_path = m_log_path.parent_path() / "_key_logger_tmp_file_new";
            const auto old_file_tmp_path = m_log_path.parent_path() / "_key_logger_tmp_file_original";
            try {

                std::fstream tmp_file = get_log_file(new_file_path);
                for (const auto &item: log_map) {
                    tmp_file << operation::INSERT << item.first << std::hex << std::setw(sizeof(item.second)) << item.second << std::endl;
                }
                tmp_file.close();
                m_log_file.close();

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
            m_log_file.seekp(0, std::ios::end);
            std::filesystem::remove (old_file_tmp_path);
        }

        static std::fstream get_log_file (const std::filesystem::path& path) {
            if (std::filesystem::exists (path)) {
                return {path, std::ios::in | std::ios::out};
            }
            else {
                return {path, std::ios::in | std::ios::out | std::ios::trunc};
            }
        }
    };
} // end namespace uh::cluster


#endif //CORE_KEY_LOGGER_H
