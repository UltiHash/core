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
        m_file.write(reinterpret_cast <const char *>(&m_pos), END_POINTER_SIZE);
        m_total_free_size += size;
    }

    std::optional <wide_span> pop_free_spot () {
        if (m_pos + m_pop_count == END_POINTER_SIZE) {
            return std::nullopt;
        }

        m_file.seekg (END_POINTER_SIZE + m_pop_count * ELEMENT_SIZE);
        unsigned long buffer [3];
        m_file.read(reinterpret_cast<char *> (&buffer), sizeof (buffer));
        wide_span wspan {{buffer[0], buffer[1]}, buffer[2]};
        m_pop_count ++;

        return wspan;

    }

    void apply_popped_items () {
        // TODO what to do about increasing zeros offset
        //m_total_free_size = m_total_free_size - size;
        m_pop_count = 0;
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
            file.read(reinterpret_cast <char *>(&m_pos), END_POINTER_SIZE);
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
            file.write(reinterpret_cast <const char *>(&m_pos), END_POINTER_SIZE);
            return file;
        }
    }

    uint128_t get_total_free_size() {
        ospan <char> buffer (m_pos - END_POINTER_SIZE);
        m_file.seekg(END_POINTER_SIZE);
        m_file.read(buffer.data.get(), static_cast <long> (buffer.size));
        uint128_t total_size = 0;
        for (std::size_t i = 0; i < buffer.size; i += ELEMENT_SIZE) {
            total_size += *reinterpret_cast <unsigned long*> (buffer.data.get() + i + 2 * sizeof(unsigned long));
        }
        return total_size;
    }

    void shift_forward () {
        if (m_pos == END_POINTER_SIZE) {
            return;
        }
        long offset = END_POINTER_SIZE;
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

        if (offset - data_size > END_POINTER_SIZE
            and free_spot_data[2] != 0) {

            ospan <char> buffer (m_pos - offset + data_size);
            std::memcpy (buffer.data.get(), &free_spot_data, data_size);
            m_file.read(buffer.data.get() + data_size, static_cast <long> (buffer.size - data_size));
            m_file.seekp(0);
            m_pos = static_cast <long> (END_POINTER_SIZE + buffer.size);
            m_file.write(reinterpret_cast<const char *>(&m_pos), END_POINTER_SIZE);
            m_file.write(buffer.data.get(), static_cast <long> (buffer.size));
            std::filesystem::resize_file(m_hole_log, m_pos);
        }
        else if (offset - data_size > END_POINTER_SIZE
            and free_spot_data[2] == 0) {
            m_file.seekp(0);
            m_pos = END_POINTER_SIZE;
            m_file.write(reinterpret_cast<const char *>(&m_pos), END_POINTER_SIZE);
            std::filesystem::resize_file(m_hole_log, m_pos);
        }
    }

    const std::filesystem::path m_hole_log;
    std::fstream m_file;
    uint128_t m_total_free_size;
    long m_pop_count;
    long m_pos;
    constexpr static long END_POINTER_SIZE = sizeof (m_pos);
    constexpr static long ELEMENT_SIZE = 3 * sizeof (unsigned long);
};

} // end namespace uh::cluster

#endif //CORE_FREE_SPOT_MANAGER_H
