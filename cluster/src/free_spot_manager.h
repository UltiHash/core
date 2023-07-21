//
// Created by masi on 7/21/23.
//

#ifndef CORE_FREE_SPOT_MANAGER_H
#define CORE_FREE_SPOT_MANAGER_H

#include <filesystem>
#include <fstream>
#include <optional>
#include "common.h"

namespace uh::cluster {

class free_spot_manager {
public:
    explicit free_spot_manager (std::filesystem::path hole_log):
        m_hole_log(std::move (hole_log)),
        m_file (open_file()) {


    }

    void push_free_spot (uint128_t, std::size_t size) {
/*
        unsigned long free_spot_data[3];
        free_spot_data[0] = pointer.get_data()[0];
        free_spot_data[1] = pointer.get_data()[1];
        free_spot_data[2] = size;

        long log_pos;

        ::lseek(m_log_fd, 0, SEEK_SET);
        ::read(m_log_fd, &log_pos, sizeof(log_pos));
        ::lseek(m_log_fd, log_pos, SEEK_SET);
        ::write(m_log_fd, free_spot_data, sizeof(free_spot_data));

        log_pos += sizeof(free_spot_data);
        ::lseek(m_log_fd, 0, SEEK_SET);
        ::write(m_log_fd, &log_pos, sizeof(log_pos));
        */
    }

    std::optional <wide_span> pop_free_spot () {
        /*
         *
        unsigned long free_spot_data[3];
        long log_pos;
        ::lseek(m_log_fd, 0, SEEK_SET);
        ::read(m_log_fd, &log_pos, sizeof(log_pos));
        auto element_index = log_pos - static_cast <long> (sizeof (free_spot_data));
        if (element_index > 0) {

            ::lseek(m_log_fd, element_index, SEEK_SET);
            size_t allocated = 0;
            while (element_index > sizeof (log_pos) and allocated < size) {
                element_index -= sizeof (free_spot_data);
                ::lseek(m_log_fd, element_index, SEEK_SET);
            }

        }
         */
        return {};
    }

    void sync () {
        m_file.sync();
    }

    ~free_spot_manager() {
        sync();
    }

private:

    std::fstream open_file () {
        if (std::filesystem::exists(m_hole_log)) {
            std::fstream file {m_hole_log, std::ios::binary | std::ios::in | std::ios::out};
            if (!file) {
                throw std::filesystem::filesystem_error ("Could not open/create the log file in the data store root",
                                                         std::error_code(errno, std::system_category()));
            }
            return file;
        }
        else {
            std::filesystem::create_directories(m_hole_log);
            std::fstream file {m_hole_log, std::ios::binary | std::ios::in | std::ios::out | std::ios::trunc};
            if (!file) {
                throw std::filesystem::filesystem_error ("Could not open/create the log file in the data store root",
                                                         std::error_code(errno, std::system_category()));
            }
            long pos = sizeof(long);
            file.write(reinterpret_cast<const char *>(&pos), sizeof(long));
            return file;
        }
    }

    const std::filesystem::path m_hole_log;
    std::fstream m_file;
};

} // end namespace uh::cluster

#endif //CORE_FREE_SPOT_MANAGER_H
