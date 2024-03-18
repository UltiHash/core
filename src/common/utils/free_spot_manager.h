#ifndef CORE_FREE_SPOT_MANAGER_H
#define CORE_FREE_SPOT_MANAGER_H

#include "common/types/big_int.h"
#include "common/types/common_types.h"
#include <cstring>
#include <filesystem>
#include <forward_list>
#include <fstream>
#include <optional>

namespace uh::cluster {

class free_spot_manager {
public:
    explicit free_spot_manager(std::filesystem::path hole_log, size_t minimum_size);


    /**
     * Will take a note about the given free spot, without actually applying it
     * Free spot size will not be affected yet.
     * @param pointer
     * @param size
     */
    void note_free_spot(uint128_t pointer, std::size_t size);

    /**
     * Applies the pending/noted free spots.
     * Free spot size will be updated accordingly
     */
    void push_noted_free_spots();

    /**
     * Inserts a new free spot and updates the free spot size
     * @param pointer
     * @param size
     */
    void push_free_spot(uint128_t pointer, std::size_t size);

    /**
     * Takes a note of removing the most recent inserted free spot
     * Free spot size will not be affected yet.
     *
     * @return a fragment containing the popped spot
     */
    std::optional<fragment> pop_free_spot();

    /**
     * Applies the pending/noted removed free spots and updates the free spot size
     */
    void apply_popped_items();

    /**
     * @return total free spot size
     */
    [[nodiscard]] uint128_t total_free_spots() const noexcept;

    /**
     * Makes sure that the changes to the free spot log are persisted on disk
     */
    void sync();

    ~free_spot_manager();

private:
    std::fstream open_file();

    uint128_t get_total_free_size();

    void shift_forward();

    static void read_size(std::fstream& file, long size, char* buffer);

    const std::filesystem::path m_hole_log;
    long m_end_pos{};
    std::fstream m_file;
    uint128_t m_total_free_size;
    long m_pop_count{};
    uint128_t m_popped_size{};
    long m_start_pos{};
    const size_t m_minimum_size;
    constexpr static long ELEMENT_SIZE = 3 * sizeof(unsigned long);
    std::forward_list<std::pair<uint128_t, size_t>> m_noted_free_spots;
};

} // end namespace uh::cluster

#endif // CORE_FREE_SPOT_MANAGER_H
