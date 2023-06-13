//
// Created by masi on 23.03.23.
//

#include "utils.h"


using namespace uh::uhv;

namespace uh::fuse
{

// ---------------------------------------------------------------------

context* get_context()
{
    return static_cast<context*>(fuse_get_context()->private_data);
}

// ---------------------------------------------------------------------

void set_metadata(struct fuse_file_info* fi, f_meta_data& fmd)
{
    fi->fh = reinterpret_cast<size_t>(&fmd);
}

// ---------------------------------------------------------------------

std::vector<std::filesystem::path> get_files(
    const std::string& directory,
    const std::unordered_map <std::string, ts_f_meta_data>& metadata_list)
{
    std::vector<std::filesystem::path> files;

    for (const auto& md : metadata_list)
    {
        const auto path = std::filesystem::path(md.first);

        if (path.parent_path() == directory && !path.filename().empty())
        {
            files.emplace_back(path);
        }
    }

    return files;
}

// ---------------------------------------------------------------------

uint64_t upload_data(uh::protocol::client_pool::handle& client_handle,
                     size_t chunk_size,
                     const std::span<char>& data,
                     std::vector<char>& hashes)
{
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

}
