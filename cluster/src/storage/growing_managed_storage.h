//
// Created by masi on 5/25/23.
//

#ifndef CORE_GROWING_MANAGED_STORAGE_H
#define CORE_GROWING_MANAGED_STORAGE_H

#include <optional>

#include "../cluster_config.h"
#include "storage_common.h"

namespace uh::cluster::storage {

class growing_managed_storage: public managed_storage {

public:
    growing_managed_storage (data_store_config);

    offset_ptr allocate (std::size_t size) override;

    /** Deallocate the memory pointed by p for size number of bytes.
     *  @throws bad_alloc
     */
    void deallocate (const offset_ptr&, size_t size) override;

    /** Flush changes to the memory to disk. Only return when sync was finished.
     *  @throws on error
     */
    void sync (void* ptr, std::size_t size) override;

    /**
     * Flushes the whole mmap storage to disk. Only return when sync was finished.
     */
    void sync () override;

    /**
     * Transforms the given offset to a pointer on the memory.
     * @param offset
     * @return pointer
     */
    void* get_raw_ptr (size_t offset) override;

    ~growing_managed_storage();

private:

    offset_ptr do_allocate (size_t bytes);

    void do_deallocate (const offset_ptr& , size_t bytes);

    void load_data_store ();

    std::filesystem::path get_file_name (uint64_t offset, size_t file_size);

    static std::pair <uint64_t, size_t> parse_file_name (const std::filesystem::path& file);

    std::fstream create_logger () const;

    void replay_logger ();

    void grow ();

    constexpr static unsigned int MAX_GROW_ATTEMPTS = 5;
    const size_t m_min_file_size;
    const size_t m_max_file_size;
    const std::filesystem::path m_directory;
    std::filesystem::path m_log_file_path;

    std::fstream m_log;
    std::mutex m_mutex;
    std::size_t m_storage_size {};
    const std::size_t m_max_storage_size;
};
} // end namespace uh::cluster::storage

#endif //CORE_GROWING_MANAGED_STORAGE_H
