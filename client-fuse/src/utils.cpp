//
// Created by masi on 23.03.23.
//

#include "utils.h"

namespace uh::uhv {


private_context* get_context()
{
    return static_cast<private_context*>(fuse_get_context()->private_data);
}

f_meta_data &get_metadata(struct fuse_file_info* fi)
{
    return reinterpret_cast<ts_f_meta_data*>(fi->fh)->get()();
}

std::vector <std::string> get_files (const std::string &directory, const std::unordered_map <std::string, uh::uhv::ts_f_meta_data> &metadata_list) {
    std::vector <std::string> files;
    for (const auto& md: metadata_list) {
        const auto path = std::filesystem::path(md.first);

        if (path.parent_path() == directory && !path.filename().empty()) {
            files.emplace_back(path.filename());
        }
    }
    return files;
}

}
