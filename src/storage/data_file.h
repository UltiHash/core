#pragma once

#include <common/utils/common.h>

#include <filesystem>
#include <span>

namespace uh::cluster {

class data_file {
public:
    /**
     * Create new data_file from an existing path
     *
     * @param root base path of the file. data will be stored in `root /
     * ".data"` metadata will be stored in `root / ".meta"`.
     */
    data_file(const std::filesystem::path& root);
    data_file(data_file&& other);

    ~data_file();

    data_file(const data_file&) = delete;
    data_file& operator=(const data_file&) = delete;

    /**
     * write data to given offset
     * - do not write beyond the end of file (write pointer)
     */
    std::size_t write(std::size_t offset, std::span<const char> buffer);

    /**
     * read data from given offset
     * - do not read beyond the end of file (write pointer)
     * - no check whether data has been written before
     */
    std::size_t read(std::size_t offset, std::span<char> buffer);

    /**
     * Reserve `size` bytes of space, returning offset to write to.
     * If `size > free()` this function will not return an error, however calls
     * to `write(offset, ...)` may write less bytes than allocated.
     *
     * @note this function must not be used from concurrent threads
     */
    std::size_t alloc(std::size_t size);

    /**
     * Free `size` bytes of used space at the given `offset`.
     */
    void release(std::size_t offset, std::size_t size);

    /// persist changes to disk
    void sync();

    /// total size of file
    std::size_t filesize() const;

    /// free space in file
    std::size_t free() const;

    /// size of data
    std::size_t used_space() const;

    const std::filesystem::path& basename() const;

    /**
     * Create a new data_file at path.
     */
    static data_file create(const std::filesystem::path& root,
                            std::size_t size);

    static constexpr const char* EXTENSION_DATA_FILE = ".data";
    static constexpr const char* EXTENSION_META_FILE = ".meta";

private:
    void read_metadata();
    void write_metadata();

    int m_fd;
    int m_meta_fd;

    std::atomic<std::size_t> m_pointer;
    std::atomic<std::size_t> m_used;
    std::size_t m_filesize;
    std::filesystem::path m_path;
};

} // namespace uh::cluster
