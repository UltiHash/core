//
// Created by masi on 5/19/23.
//

#ifndef CORE_MMAP_STORAGE2_H
#define CORE_MMAP_STORAGE2_H

#include <numeric>
#include <filesystem>
#include <forward_list>

struct file_mmap_info {
    std::filesystem::path path;
    std::size_t max_size;
};

class mmap_storage {

public:

    explicit mmap_storage (const std::forward_list<file_mmap_info> &files) {
        auto total_size = std::accumulate(files.cbegin(),
                                          files.cend(),
                                          0ul,
                                          [] (const auto s, const auto& f) {
                                                return s + f.max_size;
                                            });
        const auto fd = open(file.path.c_str(), O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    }

    /** Allocate new memory in the mmap_storage and return a pointer to it.
     *  @throws bad_alloc
     */
    void* allocate (std::size_t size);

    /** Deallocate the memory pointed by p for size number of bytes.
     *  @throws bad_alloc
     */
    void deallocate (void *p, size_t size);




    /** Flush changes to the memory to disk. Only return when sync was finished.
     *  @throws on error
     */
    void sync (void *ptr, std::size_t size);

};

#endif //CORE_MMAP_STORAGE2_H
