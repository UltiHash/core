#pragma once

#include "reference_counter.h"

#include <storage/data_file.h>
#include <storage/interfaces/data_store.h>

#include <filesystem>
#include <list>
#include <shared_mutex>

namespace uh::cluster {

class default_data_store : public data_store {

public:
    default_data_store(data_store_config conf,
                       const std::filesystem::path& working_dir,
                       uint32_t service_id);

    /**
     * @brief Allocates the specified size of storage space in the data store.
     * @param size
     * @return local pointer to the allocated storage space and size of
     * allocation
     */
    allocation_t allocate(std::size_t size,
                          std::size_t alignment = DEFAULT_PAGE_SIZE);

    /**
     * Writes data to persistent storage. On completion, the provided data
     * is guaranteed to be written to persistent storage.
     *
     * @affects get_used_space()
     * @affects get_available_space()
     *
     * @param allocation: allocation to which data is written
     * @param buffers: buffers to be written
     * @param offsets: offsets of fragments in the buffer
     * @return allocated address
     */
    address write(const allocation_t allocation,
                  const std::vector<std::span<const char>>& buffers,
                  std::span<const std::size_t> offsets = {});

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
    std::size_t read(std::size_t local_pointer, std::span<char> buffer);

    /**
     * @brief Creates a reference to one or multiple storage locations.
     * Invalid/non-existing fragments will be reported as rejected fragments
     * in a returned address.
     * @param address: storage locations that are to be referenced.
     * @param count: number of references to register
     * @return an address containing rejected fragments.
     */
    address link(const address& addr, const std::size_t count = 1);

    /**
     * @brief Removes a reference to one or multiple storage locations.
     * If a storage location is no longer referenced, it is deleted and the
     * space it was using is made available for reuse.
     * @param address: storage locations that are to be unreferenced.
     * @param count: number of references to deregister
     * @return number of bytes freed in response to removing references.
     */
    std::size_t unlink(const address& addr, const std::size_t count = 1);

    /**
     * @brief Returns the current used space of the data store.
     * @return size_t: the used space in the data store
     */
    std::size_t get_used_space() const noexcept;

    /**
     * @brief Returns the current available space in the data store. Available
     * = allocated - used
     * @return size_t: the available space in the data store
     */
    std::size_t get_available_space() const noexcept;

    /**
     * @brief Returns the current write offset in the data store.
     * @return size_t: the write offset in the data store
     */
    std::size_t get_write_offset() const noexcept;

    void set_write_offset(std::size_t val) noexcept;

    ~default_data_store();

private:
    struct location {
        data_file& file;
        std::size_t offset;
    };

    struct alloc_t {
        location l;
        std::size_t size;
        std::size_t local;
    };

    void sync();

    void maintain_refcount(const std::size_t local_pointer,
                           const std::size_t size,
                           std::span<const std::size_t> offsets);

    void allocate_files(std::size_t offset, std::size_t size);

    location file_location(size_t pointer);

    std::size_t fetch_used_space() const;

    size_t internal_delete(std::size_t offset, std::size_t size);

    int open_metadata(const std::filesystem::path& path);
    void read_metadata();
    void write_metadata();

    const uint32_t m_storage_id;
    const std::filesystem::path m_root;
    data_store_config m_conf;
    const std::size_t m_filesize;

    std::vector<data_file> m_files;
    std::size_t m_file_count;

    int m_meta_fd;

    mutable std::shared_mutex m_mutex;
    std::size_t m_used_space{};
    std::size_t m_write_offset{};

    reference_counter m_refcounter;
};

} // end namespace uh::cluster
