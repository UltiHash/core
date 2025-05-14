#include "data_file.h"

#include <common/telemetry/log.h>
#include <common/utils/error.h>
#include <common/utils/io.h>

#include <fcntl.h>
#include <unistd.h>

#include <fstream>

namespace uh::cluster {

namespace {

struct metadata {
    std::size_t used = 0ull;
    std::size_t filesize = 0ull;
};

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
      m_used(other.m_used.load()),
      m_filesize(std::move(other.m_filesize)),
      m_path(std::move(other.m_path)) {

    other.m_fd = -1;
    other.m_meta_fd = -1;
    other.m_used = 0ull;
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
    std::size_t size = std::min(m_filesize - offset, buffer.size());
    m_used += size;
    return safe_pwrite(m_fd, buffer.subspan(0, size), offset);
}

std::size_t data_file::read(std::size_t offset, std::span<char> buffer) {
    return safe_pread(
        m_fd, buffer.subspan(0, std::min(m_filesize - offset, buffer.size())),
        offset);
}

std::size_t data_file::release(std::size_t offset, std::size_t size) {

    std::size_t count = std::min(m_filesize - offset, size);
    if (fallocate(m_fd, FALLOC_FL_PUNCH_HOLE | FALLOC_FL_KEEP_SIZE, offset,
                  count)) {
        throw_from_errno("could not free space for " + m_path.string());
    }

    m_used -= count;
    return count;
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
    metadata md{.used = 0ull, .filesize = size};

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

    m_used = md.used;
    m_filesize = md.filesize;
}

void data_file::write_metadata() {
    metadata md{.used = m_used, .filesize = m_filesize};

    safe_pwrite(m_meta_fd,
                std::span<const char>(reinterpret_cast<const char*>(&md),
                                      sizeof(metadata)),
                0);
}

} // namespace uh::cluster
