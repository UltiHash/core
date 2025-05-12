#pragma once

#include <filesystem>
#include <span>

namespace uh::cluster {

/**
 * Read up to `buffer.size()` bytes from file descriptor `fd` and return the
 * number of read bytes. Throws in case of an error.
 *
 * This function takes care of handling partial reads and repeats the read
 * request until either all bytes have been read, the end of file is reached or
 * an error occurred.
 *
 * @return number of bytes read
 */
std::size_t safe_pread(int fd, std::span<char> buffer, std::size_t offset);

/**
 * Write up to `buffer.size()` bytes to file descriptor `fd` and return the
 * number of written bytes. Throws in case of an error.
 *
 * This function takes care of handling partial writes and repeats the write
 * request until either all bytes have been written or an error occurred.
 *
 * @return number of bytes written
 */
std::size_t safe_pwrite(int fd, std::span<const char> buffer,
                        std::size_t offset);

/**
 * Concatenate a string to a path.
 *
 * @param p path
 * @param s string to be concatenated
 * @return concatenated path
 */
std::filesystem::path operator+(const std::filesystem::path& p, std::string s);

/**
 * Open an existing file at the given path and return the file descriptor.
 * Throws in case of an error.
 *
 * @param path path to the file
 * @return file descriptor
 */
int open_file(const std::filesystem::path& path);

} // namespace uh::cluster
