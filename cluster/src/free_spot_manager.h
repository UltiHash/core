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

        m_file.seekp(m_end_pos);
        unsigned long free_spot_data[3];
        free_spot_data[0] = pointer.get_data()[0];
        free_spot_data[1] = pointer.get_data()[1];
        free_spot_data[2] = size;
        write_size(m_file, ELEMENT_SIZE, reinterpret_cast<const char *>(free_spot_data));
        m_end_pos += sizeof(free_spot_data);
        m_total_free_size += size;
    }

    std::optional <wide_span> pop_free_spot () {
        if (m_start_pos + m_pop_count * ELEMENT_SIZE == m_end_pos) {
            return std::nullopt;
        }

        m_file.seekg (m_start_pos + m_pop_count * ELEMENT_SIZE);
        unsigned long buffer [3];
        read_size (m_file, ELEMENT_SIZE, reinterpret_cast <char *> (&buffer));
        wide_span wspan {{buffer[0], buffer[1]}, buffer[2]};
        m_pop_count ++;

        return wspan;
    }

    void apply_popped_items () {
        m_file.seekg (m_start_pos);
        const auto free_size = m_pop_count * ELEMENT_SIZE;
        const auto zeros = std::unique_ptr <char> (new char [free_size]);
        write_size (m_file, free_size, zeros.get());
        m_start_pos += free_size;
        m_pop_count = 0;
        m_total_free_size = m_total_free_size - free_size;
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
            m_end_pos = static_cast <long> (std::filesystem::file_size(m_hole_log));
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
            m_end_pos = 0;
            return file;
        }
    }

    uint128_t get_total_free_size() {
        ospan <char> buffer (m_end_pos);
        m_file.seekg(0);
        read_size (m_file, static_cast <long> (buffer.size), buffer.data.get());
        uint128_t total_size = 0;

        for (std::size_t i = 0; i < buffer.size; i += ELEMENT_SIZE) {
            total_size += *reinterpret_cast <unsigned long*> (buffer.data.get() + i + 2 * sizeof(unsigned long));
        }
        return total_size;
    }

    void shift_forward () {
        if (m_end_pos == 0) {
            return;
        }
        long offset = 0;
        m_file.seekg(offset);
        unsigned long free_spot_data[3];
        do {
            read_size(m_file, ELEMENT_SIZE, reinterpret_cast<char *>(free_spot_data));
            offset += ELEMENT_SIZE;
        } while (free_spot_data[2] == 0 and offset < m_end_pos);
        offset -= ELEMENT_SIZE;

        if (offset > 0 and free_spot_data[2] != 0) {

            ospan <char> buffer (m_end_pos - offset);
            std::memcpy (buffer.data.get(), &free_spot_data, ELEMENT_SIZE);
            read_size(m_file, static_cast <long> (buffer.size - ELEMENT_SIZE), buffer.data.get() + ELEMENT_SIZE);
            m_file.seekp(0);
            m_end_pos = static_cast <long> (buffer.size);
            write_size(m_file, m_end_pos, buffer.data.get());
        }
        else if (offset > 0 and free_spot_data[2] == 0) {
            m_end_pos = 0;
        }
        std::filesystem::resize_file(m_hole_log, m_end_pos);

    }


    static void read_size (std::fstream& file, long size, char* buffer) {
        long r = 0;
        while (r < size) {
            file.read (buffer + r, size - r);
            r += file.gcount ();
        }
    }

    static void write_size (std::fstream& file, long size, const char* buffer) {
        long w = 0;
        while (w < size) {
            file.write (buffer + w, size - w);
            w += file.gcount ();
        }
    }

    const std::filesystem::path m_hole_log;
    std::fstream m_file;
    uint128_t m_total_free_size;
    long m_pop_count {};
    long m_start_pos {};
    long m_end_pos {};
    constexpr static long ELEMENT_SIZE = 3 * sizeof (unsigned long);
};

} // end namespace uh::cluster

#endif //CORE_FREE_SPOT_MANAGER_H
