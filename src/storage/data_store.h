#ifndef CORE_DATA_STORE_H
#define CORE_DATA_STORE_H

#include "common/utils/free_spot_manager.h"
#include <atomic>
#include <cstring>
#include <fcntl.h>
#include <list>
#include <map>
#include <memory_resource>
#include <span>
#include <unistd.h>
#include <unordered_map>
#include <unordered_set>

namespace uh::cluster {

struct data_store_config {
    std::filesystem::path working_dir;
    long file_size;
    size_t max_data_store_size;
};

class data_store {

public:
    data_store(data_store_config conf, std::size_t id, bool adaptive = true);

    /**
     * @brief Writes the data into the data store and returns the address of
     * the data written. Data might be split up and stored at different
     * locations. This is why we return an address struct which is simply a
     * collection of pointers and sizes.
     * @param data: span of characters
     * @return address: collection of pointers and sizes
     *
     * @throws std::bad_alloc: if allocated size exceeds on write.
     * @throws std::exception: corrupted storage
     *
     * @affects get_used_space()
     * @affects get_available_space()
     */
    address write(std::span<char> data);

    /**
     * @brief Read bytes of data starting from the pointer until the size and
     * store it in the buffer given.
     * @param buffer: buffer where the read data is to be written
     * @param pointer: pointer to the data which is to be read
     * @param size: number of bytes to read
     * @return std::size_t: number of read bytes
     *
     * @throws std::out_of_range invalid pointer and size given
     * @throws std::exception: corrupted storage
     */
    std::size_t read(char* buffer, uint128_t pointer, size_t size);

    /**
     * @brief Flushes modified files to disk.
     * @throws std::exception corrupted storage
     */
    void sync();

    /**
     * @brief Gives out the current used space of the data store.
     * @return size_t: the used space in the data store
     */
    [[nodiscard]] size_t get_used_space() const noexcept;

    /**
     * @brief Gives out the current available space in the data store. Available
     * = allocated - used
     * @return size_t: the available space in the data store
     */
    [[nodiscard]] size_t get_available_space() const noexcept;

    ~data_store();

private:
    struct alloc_t {
        int fd;
        long seek;
        uint128_t global_offset;
    };

    alloc_t allocate(long size);

    [[nodiscard]] std::pair<int, long>
    get_file_offset_pair(const uint128_t& pointer) const;

    [[nodiscard]] size_t fetch_used_space() const noexcept;

    int add_new_file(const uint128_t& offset, long file_size);

    [[nodiscard]] static std::pair<size_t, uint128_t>
    parse_file_name(const std::string& filename);

    [[nodiscard]] std::string get_name(const uint128_t& offset) const;

    static bool is_data_file(const std::filesystem::path& path);

    long m_last_file_data_end{};
    size_t m_data_id;
    data_store_config m_conf;
    std::vector<int> m_open_files;
    uint128_t m_global_offset;
    std::atomic<size_t> m_used;
    std::mutex m_allocate_mutex;
    std::mutex m_sync_end_offset_mutex;
};

} // end namespace uh::cluster

#endif // CORE_DATA_STORE_H
