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
#include <uhv/f_serialization.h>
#include <uhv/f_meta_data.h>
#include <fuse_f_meta_data.h>
#include <protocol/client_factory.h>
#include <protocol/client_pool.h>
#include <net/plain_socket.h>
#include <util/exception.h>

namespace uh::uhv {

struct private_context
{
    boost::asio::io_context io;
    std::unique_ptr<uh::protocol::client_pool> client_pool;
    std::unordered_map <std::string, uh::uhv::ts_f_meta_data> paths_metadata;
};


struct options
{
    const char *UHVpath;
    const char *agency_hostname;
    int agency_port;
    int agency_connections;
    bool show_help;
};

options& get_options();

int uh_getattr (const char *path, struct stat *);

void *uh_init (struct fuse_conn_info *conn);

int uh_readdir (const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi);

int uh_open (const char *path, struct fuse_file_info *fi);

int uh_read (const char *, char *, size_t, off_t, struct fuse_file_info *);

void uh_destroy (void *context);

} // end namespace uh::uhv

#endif //CORE_FUSE_OPERATIONS_H
