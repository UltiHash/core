//
// Created by masi on 7/25/23.
//
#include "free_spot_manager.h"

namespace uh::cluster {

free_spot_manager::free_spot_manager(std::filesystem::path hole_log) :
        m_hole_log(std::move (hole_log)),
        m_file (open_file()),
        m_total_free_size (get_total_free_size ()){

    shift_forward ();

    m_file.exceptions(std::ios::badbit | std::ios::failbit);
}

void free_spot_manager::push_free_spot(uint128_t pointer, std::size_t size) {

    shift_forward ();

    m_file.seekp(m_end_pos, std::ios::beg);
    unsigned long free_spot_data[3];
    free_spot_data[0] = pointer.get_data()[0];
    free_spot_data[1] = pointer.get_data()[1];
    free_spot_data[2] = size;

    m_file.write (reinterpret_cast<const char *>(&free_spot_data), ELEMENT_SIZE);

    m_end_pos += sizeof(free_spot_data);
    m_total_free_size += size;
}

std::optional<wide_span> free_spot_manager::pop_free_spot() {
    if (m_start_pos + m_pop_count * ELEMENT_SIZE == m_end_pos) {
        return std::nullopt;
    }

    m_file.seekg (m_start_pos + m_pop_count * ELEMENT_SIZE, std::ios::beg);
    unsigned long buffer [3];
    read_size (m_file, ELEMENT_SIZE, reinterpret_cast <char *> (&buffer));
    wide_span wspan {{buffer[0], buffer[1]}, buffer[2]};
    m_pop_count ++;
    m_popped_size += buffer[2];
    return wspan;
}

void free_spot_manager::apply_popped_items() {
    if (m_pop_count == 0) {
        return;
    }
    m_file.seekg (m_start_pos, std::ios::beg);
    const auto free_size = m_pop_count * ELEMENT_SIZE;
    const auto zeros = std::unique_ptr <char []> (new char [free_size]);
    std::memset(zeros.get(), 0, free_size);
    m_file.write (zeros.get(), free_size);
    m_start_pos += free_size;
    m_pop_count = 0;
    m_total_free_size = m_total_free_size - m_popped_size;
    m_popped_size = 0;
}

uint128_t free_spot_manager::total_free_spots() const noexcept {
    return m_total_free_size;
}

void free_spot_manager::sync() {
    m_file.sync();
}

free_spot_manager::~free_spot_manager() {
    sync();
}

std::fstream free_spot_manager::open_file() {
    if (std::filesystem::exists(m_hole_log)) {
        std::fstream file {m_hole_log, std::ios::binary | std::ios::in | std::ios::out};
        if (!file) {
            throw std::filesystem::filesystem_error ("Could not open/create the log file in the data store root",
                                                     std::error_code(errno, std::system_category()));
        }
        m_end_pos = static_cast <long> (std::filesystem::file_size(m_hole_log));
        return file;
    }
    else {
        if (!m_hole_log.parent_path().empty()) {
            std::filesystem::create_directories(m_hole_log.parent_path());
        }

        std::fstream file {m_hole_log, std::ios::binary | std::ios::in | std::ios::out | std::ios::trunc};
        if (!file) {
            throw std::filesystem::filesystem_error ("Could not open/create the log file in the data store root",
                                                     std::error_code(errno, std::system_category()));
        }
        m_end_pos = 0;
        return file;
    }
}

uint128_t free_spot_manager::get_total_free_size() {
    const auto buffer = std::make_unique_for_overwrite <char []>(m_end_pos);
    m_file.seekg (0, std::ios::beg);
    read_size (m_file, m_end_pos, buffer.get());
    uint128_t total_size = 0;
    const auto* long_buffer = reinterpret_cast <unsigned long*> (buffer.get());

    for (std::size_t i = 2; i < m_end_pos / sizeof (unsigned long); i += 3) {
        total_size += long_buffer[i];
    }
    return total_size;
}

void free_spot_manager::shift_forward() {
    if (m_end_pos == 0) {
        return;
    }
    long offset = 0;
    m_file.seekg (offset, std::ios::beg);
    unsigned long free_spot_data[3];
    do {
        read_size(m_file, ELEMENT_SIZE, reinterpret_cast<char *>(free_spot_data));
        offset += ELEMENT_SIZE;
    } while (free_spot_data[2] == 0 and offset < m_end_pos);
    offset -= ELEMENT_SIZE;

    if (offset > 0 and free_spot_data[2] != 0) {
        const auto buffer_size = m_end_pos - offset;
        const auto buffer = std::make_unique_for_overwrite <char []>(buffer_size);
        std::memcpy (buffer.get(), &free_spot_data, ELEMENT_SIZE);
        read_size(m_file, buffer_size - ELEMENT_SIZE, buffer.get() + ELEMENT_SIZE);
        m_file.seekp(0, std::ios::beg);
        m_end_pos = buffer_size;
        m_file.write(buffer.get(), m_end_pos);
    }
    else if (offset > 0 and free_spot_data[2] == 0) {
        m_end_pos = 0;
    }
    std::filesystem::resize_file(m_hole_log, m_end_pos);
    m_start_pos = 0;

}

void free_spot_manager::read_size(std::fstream &file, long size, char *buffer) {
    long r = 0;
    while (r < size) {
        file.read (buffer + r, size - r);
        r += file.gcount ();
    }
}

} // end namespace uh::cluster
