//
// Created by masi on 23.03.23.
//

#ifndef CORE_FUSE_OPERATIONS_H
#define CORE_FUSE_OPERATIONS_H

#include "config.hpp"
#include <fuse.h>
#include <filesystem>
#include <logging/logging_boost.h>
#include <unordered_map>
#include <uhv/job_queue.h>
#include <uhv/serialization.h>
#include <uhv/meta_data.h>
#include <protocol/client_factory.h>
#include <protocol/client_pool.h>
#include <net/plain_socket.h>
#include <util/exception.h>
#include "thread_safe_type.h"

namespace uh::uhv {

struct private_context
{
    unsigned long fd = 0;
    boost::asio::io_context io;
    std::unique_ptr<uh::protocol::client_pool> client_pool;
    ts_container fmetadata_map;
    thread_safe_type <std::unordered_map <std::string, unsigned long>> subdirectory_counts;
    thread_safe_type <std::unordered_map <unsigned long, ts_f_meta_data&>> open_files;
};


struct options
{
    const char *UHVpath;
    const char *agency_hostname;
    int agency_port;
    int agency_connections;
    bool show_help;
};

constexpr std::uint64_t max_chunk_size = 1 << 22;
constexpr std::uint64_t min_chuck_size = 64 * 1024ul;

options& get_options();

int uh_getattr (const char *path, struct stat *);

int uh_unlink(const char *path);

int uh_rmdir (const char *path);

void *uh_init (struct fuse_conn_info *conn);

int uh_readdir (const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi);

int uh_open (const char *path, struct fuse_file_info *fi);

int uh_release (const char *path, struct fuse_file_info *fi);

int uh_read (const char *, char *, size_t, off_t, struct fuse_file_info *);

int uh_write (const char *, const char *, size_t, off_t, struct fuse_file_info *);

void uh_destroy (void *context);

int uh_ftruncate (const char *path, off_t off, struct fuse_file_info *);

int uh_create (const char *path, mode_t mode, struct fuse_file_info *fi);

int uh_utimens (const char *path, const struct timespec tv[2]);

int uh_mkdir (const char *path, mode_t mode);

int uh_write_buf (const char *, struct fuse_bufvec *buf, off_t off, struct fuse_file_info *);

int uh_read_buf (const char *, struct fuse_bufvec **bufp, size_t size, off_t off, struct fuse_file_info *);

} // end namespace uh::uhv

#endif //CORE_FUSE_OPERATIONS_H
