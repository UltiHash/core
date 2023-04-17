//
// Created by masi on 23.03.23.
//

#ifndef CORE_UTILS_H
#define CORE_UTILS_H

#include <cstring>
#include <string>
#include <vector>
#include <span>
#include <uhv/f_meta_data.h>
#include <fuse.h>
#include "protocol/client_pool.h"
#include "uhv/f_serialization.h"
#include "thread_safe_type.h"


namespace uh::uhv {

struct private_context;

private_context* get_context();
f_meta_data& get_metadata(struct fuse_file_info* fi);
void set_metadata(struct fuse_file_info* fi, f_meta_data& fmd);
std::vector <std::filesystem::path> get_files (const std::string &directory, const std::unordered_map <std::string, uh::uhv::ts_f_meta_data> &metadata_list);
uint64_t upload_data (uh::protocol::client_pool::handle& client_handle, size_t chunk_size, const std::span <char> &data, std::vector <char> &hashes);
void write_metadata (std::ofstream &UHV_file, const uh::uhv::f_meta_data &md);
void rewrite_uhv_file (const std::filesystem::path &uhv_file, std::unordered_map <std::string, ts_f_meta_data> &metadata_list);

}

#endif //CORE_UTILS_H
