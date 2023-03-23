//
// Created by masi on 23.03.23.
//

#ifndef CORE_UTILS_H
#define CORE_UTILS_H

#include <cstring>
#include <string>
#include <vector>
#include <uhv/f_meta_data.h>
#include <fuse.h>


namespace uh::uhv {

struct private_context;

private_context* get_context();
f_meta_data* get_metadata(struct fuse_file_info* fi);
std::vector <std::string> get_files (const std::string &directory, const std::unordered_map <std::string, uh::uhv::f_meta_data> &metadata_list);

}

#endif //CORE_UTILS_H
