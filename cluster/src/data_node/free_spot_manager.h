//
// Created by masi on 7/21/23.
//

#ifndef CORE_FREE_SPOT_MANAGER_H
#define CORE_FREE_SPOT_MANAGER_H

#include <filesystem>
#include <fstream>
#include <optional>
#include <cstring>
#include <forward_list>
#include "common/common.h"

namespace uh::cluster {

class free_spot_manager {
public:

    explicit free_spot_manager (std::filesystem::path hole_log);

    void note_free_spot (uint128_t pointer, std::size_t size);

    void push_noted_free_spots ();

    void push_free_spot (uint128_t pointer, std::size_t size);

    std::optional <fragment> pop_free_spot ();

    void apply_popped_items ();

    [[nodiscard]] uint128_t total_free_spots () const noexcept;

    void sync ();

    ~free_spot_manager();

private:

    std::fstream open_file ();

    uint128_t get_total_free_size ();

    void shift_forward ();

    static void read_size (std::fstream& file, long size, char* buffer);

    const std::filesystem::path m_hole_log;
    long m_end_pos {};
    std::fstream m_file;
    uint128_t m_total_free_size;
    long m_pop_count {};
    uint128_t m_popped_size {};
    long m_start_pos {};
    constexpr static long ELEMENT_SIZE = 3 * sizeof (unsigned long);
    std::forward_list <std::pair <uint128_t, size_t>> m_noted_free_spots;
};

} // end namespace uh::cluster

#endif //CORE_FREE_SPOT_MANAGER_H
