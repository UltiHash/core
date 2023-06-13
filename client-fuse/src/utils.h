//
// Created by masi on 23.03.23.
//

#ifndef CORE_UTILS_H
#define CORE_UTILS_H

#include <protocol/client_pool.h>
#include <uhv/file.h>

#include "context.h"
#include "thread_safe_type.h"

#include <cstring>
#include <string>
#include <vector>
#include <span>

#include <fuse.h>


namespace uh::fuse
{

// ---------------------------------------------------------------------

context* get_context();

/**
 * Set the metadata for the fuse_file_info `fi`.
 */
void set_metadata(struct fuse_file_info* fi, uhv::meta_data& fmd);

// ---------------------------------------------------------------------

std::vector<std::filesystem::path> get_files(
    const std::string& directory,
    const std::unordered_map<std::string, ts_meta_data>& metadata_list);

uint64_t upload_data (uh::protocol::client_pool::handle& client_handle, size_t chunk_size, const std::span <char> &data, std::vector <char> &hashes);
void write_metadata (std::ofstream &UHV_file, const uhv::meta_data &md);

} // namespace uh::fuse

#endif //CORE_UTILS_H
