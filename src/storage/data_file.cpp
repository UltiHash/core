#include "data_file.h"

#include <common/telemetry/log.h>
#include <common/utils/error.h>
#include <common/utils/io.h>

#include <fcntl.h>
#include <unistd.h>

namespace uh::cluster {

namespace {

struct metadata {
    std::size_t pointer = 0ull;
    std::size_t used = 0ull;
    std::size_t filesize = 0ull;
};

int open_file(const std::filesystem::path& path) {
    int fd = ::open(path.c_str(), O_RDWR);
    if (fd == -1) {
        throw_from_errno("could not open file " + path.string());
    }

    return fd;
}

std::filesystem::path operator+(const std::filesystem::path& p, std::string s) {
    auto rv = p;
    rv += s;
    return rv;
}

} // namespace

data_file::data_file(const std::filesystem::path& root)
    : m_fd(open_file(root + EXTENSION_DATA_FILE)),
      m_meta_fd(open_file(root + EXTENSION_META_FILE)),
      m_path(root) {
    read_metadata();
}

data_file::data_file(data_file&& other)
    : m_fd(std::move(other.m_fd)),
      m_meta_fd(std::move(other.m_meta_fd)),
      m_pointer(other.m_pointer.load()),
      m_used(other.m_used.load()),
      m_filesize(std::move(other.m_filesize)),
      m_path(std::move(other.m_path)) {

    other.m_fd = -1;
    other.m_meta_fd = -1;
    other.m_used = 0ull;
    other.m_pointer = 0ull;
}

data_file::~data_file() {

    if (m_fd != -1) {
        try {
            sync();
        } catch (const std::exception& e) {
            LOG_WARN() << "failure syncing data file: " << e.what();
        }

        if (close(m_fd) == -1) {
            LOG_WARN() << "error closing file descriptor: " << errno_message();
        }
    }

    if (m_meta_fd != -1) {
        if (close(m_meta_fd) == -1) {
            LOG_WARN() << "error closing file descriptor: " << errno_message();
        }
    }
}

std::size_t data_file::write(std::size_t offset, std::span<const char> buffer) {

    return safe_pwrite(
        m_fd, buffer.subspan(0, std::min(filesize() - offset, buffer.size())),
        offset);
}

std::size_t data_file::read(std::size_t offset, std::span<char> buffer) {

    if (m_pointer < offset) {
        throw std::runtime_error("reading out of range");
    }

    return safe_pread(
        m_fd, buffer.subspan(0, std::min(m_pointer - offset, buffer.size())),
        offset);
}

std::size_t data_file::alloc(std::size_t size) {

    std::size_t rv = m_pointer;
    std::size_t real_size = std::min(size, m_filesize - rv);

    m_pointer += real_size;
    m_used += real_size;

    return rv;
}

void data_file::release(std::size_t offset, std::size_t size) {

    if (fallocate(m_fd, FALLOC_FL_PUNCH_HOLE | FALLOC_FL_KEEP_SIZE, offset,
                  size)) {
        throw_from_errno("could not free space for " + m_path.string());
    }

    m_used -= size;
}

void data_file::sync() {
    write_metadata();
    int fd_rv = fsync(m_fd);
    int md_rv = fsync(m_meta_fd);
    if (fd_rv == -1 || md_rv == -1) {
        throw_from_errno("data file sync failed for " + m_path.string());
    }
}

std::size_t data_file::filesize() const { return m_filesize; }

std::size_t data_file::free() const { return filesize() - m_pointer; }

std::size_t data_file::used_space() const { return m_used; }

const std::filesystem::path& data_file::basename() const { return m_path; }

data_file data_file::create(const std::filesystem::path& root,
                            std::size_t size) {
    auto data_path = root + EXTENSION_DATA_FILE;
    int fd = open(data_path.c_str(), O_RDWR | O_CREAT, S_IWUSR | S_IRUSR);
    if (fd == -1) {
        throw_from_errno("cannot create data file " + data_path.string());
    }

    int rc = ftruncate(fd, size);
    if (rc != 0) [[unlikely]] {
        close(fd);
        throw_from_errno("cannot truncate data file");
    }

    close(fd);

    auto meta_path = root + EXTENSION_META_FILE;
    metadata md{.pointer = 0ull, .used = 0ull, .filesize = size};

    {
        std::ofstream meta_file(meta_path);
        meta_file.write(reinterpret_cast<char*>(&md), sizeof(metadata));
    }

    return data_file(root);
}

void data_file::read_metadata() {
    metadata md;

    safe_pread(m_meta_fd,
               std::span<char>(reinterpret_cast<char*>(&md), sizeof(metadata)),
               0);

    m_pointer = md.pointer;
    m_used = md.used;
    m_filesize = md.filesize;
}

void data_file::write_metadata() {
    metadata md{.pointer = m_pointer, .used = m_used, .filesize = m_filesize};

    safe_pwrite(m_meta_fd,
                std::span<const char>(reinterpret_cast<const char*>(&md),
                                      sizeof(metadata)),
                0);
}

} // namespace uh::cluster
