//
// Created by masi on 7/21/23.
//

#ifndef CORE_FREE_SPOT_MANAGER_H
#define CORE_FREE_SPOT_MANAGER_H

#include <filesystem>
#include <fstream>
#include <optional>
#include <cstring>
#include "common.h"
#include "ospan.h"

namespace uh::cluster {

class free_spot_manager {
public:

    explicit free_spot_manager (std::filesystem::path hole_log):
        m_hole_log(std::move (hole_log)),
        m_file (open_file()),
        m_total_free_size (get_total_free_size ()){
    }

    void push_free_spot (uint128_t pointer, std::size_t size) {

        shift_forward ();

        m_file.seekp(m_pos);
        unsigned long free_spot_data[3];
        free_spot_data[0] = pointer.get_data()[0];
        free_spot_data[1] = pointer.get_data()[1];
        free_spot_data[2] = size;
        m_file.write(reinterpret_cast<const char *>(free_spot_data), sizeof(free_spot_data));

        m_file.seekp(0);
        m_pos += sizeof(free_spot_data);
        m_file.write(reinterpret_cast <const char *>(&m_pos), sizeof(m_pos));
        m_total_free_size += size;
    }

    std::optional <wide_span> pop_free_spot () {
        pop_count ++;
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

    void apply_popped_items () {
        //m_total_free_size = m_total_free_size - size;

    }

    uint128_t total_free_spots () {
        return m_total_free_size;
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
            file.read(reinterpret_cast <char *>(&m_pos), sizeof (m_pos));
            shift_forward ();
            return file;
        }
        else {
            std::filesystem::create_directories(m_hole_log);
            std::fstream file {m_hole_log, std::ios::binary | std::ios::in | std::ios::out | std::ios::trunc};
            if (!file) {
                throw std::filesystem::filesystem_error ("Could not open/create the log file in the data store root",
                                                         std::error_code(errno, std::system_category()));
            }
            m_pos = sizeof(long);
            file.write(reinterpret_cast <const char *>(&m_pos), sizeof(m_pos));
            return file;
        }
    }

    uint128_t get_total_free_size() {
        ospan <char> buffer (m_pos - sizeof(m_pos));
        m_file.seekg(sizeof(m_pos));
        m_file.read(buffer.data.get(), static_cast <long> (buffer.size));
        uint128_t total_size = 0;
        for (std::size_t i = 0; i < buffer.size; i += 3 * sizeof (unsigned long)) {
            total_size += *reinterpret_cast <unsigned long*> (buffer.data.get() + i + 2 * sizeof(unsigned long));
        }
        return total_size;
    }

    void shift_forward () {
        if (m_pos == sizeof (m_pos)) {
            return;
        }
        long offset = sizeof (m_pos);
        m_file.seekg(offset);
        unsigned long free_spot_data[3];
        const auto data_size = static_cast <long> (sizeof(free_spot_data));
        do {

            long w = 0;
            while (w < data_size) {
                m_file.read(reinterpret_cast<char *>(free_spot_data) + w, data_size - w);
                w += m_file.gcount();
            }

            offset += data_size;
        } while (free_spot_data[2] == 0 and offset < m_pos);

        if (offset - data_size > sizeof (m_pos)
            and free_spot_data[2] != 0) {

            ospan <char> buffer (m_pos - offset + data_size);
            std::memcpy (buffer.data.get(), &free_spot_data, data_size);
            m_file.read(buffer.data.get() + data_size, static_cast <long> (buffer.size - data_size));
            m_file.seekp(0);
            m_pos = static_cast <long> (sizeof(m_pos) + buffer.size);
            m_file.write(reinterpret_cast<const char *>(&m_pos), sizeof(m_pos));
            m_file.write(buffer.data.get(), static_cast <long> (buffer.size));
            std::filesystem::resize_file(m_hole_log, m_pos);
        }
        else if (offset - data_size > sizeof (m_pos)
            and free_spot_data[2] == 0) {
            m_file.seekp(0);
            m_pos = static_cast <long> (sizeof(m_pos));
            m_file.write(reinterpret_cast<const char *>(&m_pos), sizeof(m_pos));
            std::filesystem::resize_file(m_hole_log, m_pos);
        }
    }

    const std::filesystem::path m_hole_log;
    std::fstream m_file;
    uint128_t m_total_free_size;
    long pop_count;
    long m_pos;
};

} // end namespace uh::cluster

#endif //CORE_FREE_SPOT_MANAGER_H
