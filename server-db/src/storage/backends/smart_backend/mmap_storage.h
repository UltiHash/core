//
// Created by masi on 5/15/23.
//

#ifndef CORE_MMAP_STORAGE_H
#define CORE_MMAP_STORAGE_H

#include <filesystem>
#include <forward_list>
#include <fstream>
#include <memory_resource>
#include <map>

#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

namespace uh::dbn::storage::smart {


struct file_mmap_info {
    std::filesystem::path path;
    void *address;
    std::size_t max_size;
};

class mmap_storage {
public:
    mmap_storage (const std::forward_list<file_mmap_info> &files);

    /** Allocate new memory in the mmap_storage and return a pointer to it.
     *  @throws bad_alloc
     */
    void *allocate (std::size_t size);

    /** Deallocate the memory pointed by p for size number of butes.
     *  @throws bad_alloc
     */
    void deallocate (void *p, size_t size);


    /** Flush changes to the memory to disk. Only return when sync was finished.
     *  @throws on error
     */
    void sync (void *ptr, std::size_t size);

private:

    void *do_allocate (size_t bytes);

    void do_deallocate (void *p, size_t bytes);

    void mmap_file (const file_mmap_info &file);

    class resource_entry;
    std::fstream m_log;
    std::map <void *, resource_entry> m_resources;
    std::fstream create_logger ();
    void replay_logger ();
};

} // end namespace uh::dbn::storage::smart

#endif //CORE_MMAP_STORAGE_H
