//
// Created by masi on 10/2/23.
//

#ifndef CORE_KEY_LOGGER_H
#define CORE_KEY_LOGGER_H


#include "unordered_map"
#include <common/common.h>
#include <string>
#include <fstream>
#include <filesystem>

namespace uh::cluster {
    class log_file {

        std::filesystem::path m_log_path;
        std::fstream m_log_file;
    public:
        enum operation:uint8_t {
            INSERT_START,
            INSERT_END,
            DELETE_START,
            DELETE_END,
            UPDATE_START,

            INSERT,
        };

        log_file (std::filesystem::path log_path):
            m_log_path (std::move (log_path)),
            m_log_file (get_log_file (m_log_path)) {
            if (!m_log_file.is_open()) {
                throw std::runtime_error ("Could not open the key logger file");
            }
        }

        void append (std::span <char> key, uint64_t object_id, operation op) {

        }

        std::unordered_map <ospan<char>, uint64_t> replay () {

        }

        void recreate () {
            // not for now
        }

    private:

        std::fstream get_log_file (const std::filesystem::path& path) {
            if (std::filesystem::exists (path)) {
                return std::fstream (path);
            }
            else {
                return std::fstream (path, std::ios::in | std::ios::out | );
            }
        }
    };
} // end namespace uh::cluster


#endif //CORE_KEY_LOGGER_H
