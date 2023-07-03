//
// Created by masi on 5/15/23.
//

#ifndef CORE_FIXED_MANAGED_STORAGE_H
#define CORE_FIXED_MANAGED_STORAGE_H

#include "storage/backends/smart_backend/smart_config.h"
#include "storage_common.h"

#include <filesystem>
#include <forward_list>
#include <fstream>
#include <map>
#include <set>
#include <system_error>

#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>

namespace uh::dbn::storage::smart {

class fixed_managed_storage;
class growing_managed_storage;

class fixed_managed_storage: public managed_storage {

public:

    explicit fixed_managed_storage (fixed_managed_storage_config);

    /** Allocate new memory in the mmap_storage and return a pointer to it.
     *  @throws bad_alloc
     */
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

    std::size_t get_total_used_size () const noexcept;

    /**
     * Transforms the given offset to a pointer on the memory.
     * @param offset
     * @return pointer
     */
    void* get_raw_ptr (size_t offset) override;

    ~fixed_managed_storage();

private:

    offset_ptr do_allocate (size_t bytes);

    void do_deallocate (const offset_ptr& , size_t bytes);

    std::fstream create_logger () const;

    void replay_logger ();

    bool files_existence_consistency ();

    const fixed_managed_storage_config m_conf;
    std::fstream m_log;
    std::size_t m_aggregated_size {};
    std::size_t m_total_used_size {};
};


} // end namespace uh::dbn::storage::smart

#endif //CORE_FIXED_MANAGED_STORAGE_H
