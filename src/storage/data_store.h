#ifndef CORE_DATA_STORE_H
#define CORE_DATA_STORE_H

#include "common/types/scoped_buffer.h"

#include <condition_variable>
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
#include <filesystem>
#include "common/types/address.h"

namespace uh::cluster {

struct data_store_config {
    std::filesystem::path working_dir;
    size_t file_size;
    size_t max_data_store_size;
};

class data_store {

public:
    data_store(data_store_config conf, uint32_t service_id, uint32_t data_store_id);

    /**
     * Allocates for the given data size and stores the
     * data, the allocation, and internal allocation info in the
     * ongoing async writes queue.
     *
     * @affects get_used_space()
     * @affects get_available_space()
     *
     * @param data
     * @return  allocated address
     */
    address register_write (const shared_buffer <char>& data);

    /**
     * Allocates for the given data size and stores the
     * data, the allocation, and internal allocation info in the
     * ongoing async writes queue.
     *
     * @affects get_used_space()
     * @affects get_available_space()
     *
     * @param data
     * @return  allocated address
     */
    address register_write (const std::string_view& data);

    /**
     * Writes the data that is registered by the given address to disk.
     *
     * @param addr the address that the data is registered with
     */
    void perform_write (const address& addr);

    /**
     * Waits for completion of async write operations for the given address
     *
     * @param addr
     */
    void wait_for_ongoing_writes (const address& addr);

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
    std::size_t read(char* buffer, const uint128_t& pointer, size_t size);
    std::size_t read_up_to(char* buffer, const uint128_t& pointer, size_t size);

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
        size_t seek;
        uint128_t global_offset;
    };

    alloc_t internal_allocate(size_t size);


    std::pair <size_t, shared_buffer<char>> find_async_data (size_t pointer, size_t size);

    [[nodiscard]] std::pair<int, long>
    get_file_offset_pair(size_t pointer) const;

    [[nodiscard]] size_t fetch_used_space(const std::filesystem::path& last_file) const noexcept;

    std::filesystem::path add_new_file(size_t offset, size_t file_size);

    [[nodiscard]] static std::pair<size_t, size_t>
    parse_file_name(const std::string& filename);

    [[nodiscard]] std::string get_name(size_t offset) const;

    static bool is_data_file(const std::filesystem::path& path);

    size_t m_last_file_data_end{};
    const uint32_t m_storage_id;
    const uint32_t m_data_store_id;
    const std::filesystem::path m_root;
    data_store_config m_conf;
    std::vector<std::pair<int, size_t>> m_open_files;
    std::atomic<size_t> m_used{};
    std::mutex m_allocate_mutex;
    std::mutex m_sync_end_offset_mutex;
    std::mutex m_async_mutex;
    std::condition_variable m_async_cv;
    std::map <size_t, std::pair <alloc_t, shared_buffer<char>>> m_ongoing_async_writes;
};

} // end namespace uh::cluster

#endif // CORE_DATA_STORE_H
