//
// Created by masi on 23.03.23.
//

#include "utils.h"

#include <client/upload.h>
#include <chunking/fixed_size_chunker.h>


using namespace uh;
using namespace uh::uhv;

namespace uh::fuse
{

// ---------------------------------------------------------------------

context* get_context()
{
    return static_cast<context*>(fuse_get_context()->private_data);
}

// ---------------------------------------------------------------------

void set_metadata(struct fuse_file_info* fi, meta_data& fmd)
{
    fi->fh = reinterpret_cast<size_t>(&fmd);
}

// ---------------------------------------------------------------------

std::vector<std::filesystem::path> get_files(
    const std::string& directory,
    const std::unordered_map <std::string, ts_meta_data>& metadata_list)
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
                     std::span<char> data,
                     std::vector<char>& hashes)
{
    std::size_t effective_size = 0;
    chunking::fixed_size_chunker fsc(chunk_size);

    client::chunked_upload(*client_handle, data, fsc, effective_size);
    return effective_size;
}

// ---------------------------------------------------------------------

}
