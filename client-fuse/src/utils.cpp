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

std::vector <std::filesystem::path> get_files (const std::string &directory, const std::unordered_map <std::string,
                                               uh::uhv::ts_f_meta_data> &metadata_list) {
    std::vector <std::filesystem::path> files;
    for (const auto& md: metadata_list) {
        const auto path = std::filesystem::path(md.first);

        if (path.parent_path() == directory && !path.filename().empty()) {
            files.emplace_back(path);
        }
    }
    return files;
}

std::size_t subfolders_count (const std::string &directory, std::unordered_map <std::string,
        uh::uhv::ts_f_meta_data> &metadata_list) {
    size_t res = 0;
    for (auto &md: metadata_list) {
        const auto path = std::filesystem::path(md.first);
        if (path.parent_path() == directory and !path.filename().empty() and (md.second.get()().f_type() == uh::uhv::uh_file_type::directory)) {
            res ++;
        }
    }
    return res;
}

uint64_t upload_data (uh::protocol::client_pool::handle& client_handle, size_t chunk_size, const std::span <char> &data, std::vector <char> &hashes) {
    std::size_t write_offset = 0ul;
    auto effective_size = 0;

     do {
        auto write_size = chunk_size;
        if (write_offset + write_size > data.size()) {
            write_size = data.size() - write_offset;
        }

        auto alloc = client_handle->allocate(write_size);
        std::span <char> sbuf (const_cast <char *> (data.data() + write_offset), write_size);
        io::write_from_buffer(alloc->device(), sbuf);

        auto meta_data = alloc->persist();
        std::copy(meta_data.hash.cbegin(), meta_data.hash.cend(), std::back_inserter(hashes));
        effective_size += meta_data.effective_size;
        write_offset += write_size;
    } while (write_offset < data.size());
    return effective_size;
}


void write_metadata (std::ofstream &UHV_file, const uh::uhv::f_meta_data &md) {
    auto relative_path = (md.f_path() == "/") ? "/" : std::filesystem::relative(md.f_path(), "/");
    auto bytes = uh::uhv::f_serialization::serialize_f_meta_data(std::make_unique<uh::uhv::f_meta_data>(md),
                                                                 relative_path);
    UHV_file.write(reinterpret_cast<const char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
}

void rewrite_uhv_file (const std::filesystem::path &uhv_file, std::unordered_map <std::string, ts_f_meta_data> &data) {
    std::ofstream UHV_file(uhv_file, std::ios::trunc | std::ios::out | std::ios::binary);
    for (auto &tsmd: data) {
        auto &md = tsmd.second.get()();
        write_metadata(UHV_file, md);
    }
}

}
