//
// Created by masi on 23.03.23.
//
#include "fuse_operations.h"
#include "exception.h"
#include "utils.h"


using namespace uh::uhv;

namespace uh::fuse
{

options& get_options()
{
    static options opt;

    return opt;
}


/* --- fuse_operations core functionality --- */

int __uh_getattr(const std::filesystem::path& path, struct stat* stbuf)
{
    std::cout << "uh_getattr(" << path << ", stbuf)\n";
    memset(stbuf, 0, sizeof(struct stat));

    auto* ctx = get_context();
    auto container_handle = ctx->metadata_map();
    auto& unordered_map = container_handle();

    auto it = unordered_map.find(path);
    if (it == unordered_map.end() && path.filename().c_str()[0] == '.')
    {
        std::cout << "leaving uh_getattr(" << path << ", )\n";
        return 0;
    }

    if (it == unordered_map.end())
    {
        stbuf->st_size = 0;
        stbuf->st_nlink = 1;
        stbuf->st_blksize = 4096;
        stbuf->st_blocks = 0;
        clock_gettime(CLOCK_MONOTONIC, &stbuf->st_ctim);
        stbuf->st_mode = S_IFREG | 0666;

        THROW(exception, "file not found", ENOENT);
    }

    // !!! container should be released here since mera_data is found
    auto meta_handle = it->second.get();
    auto& meta_data = meta_handle();

    uhv::uh_file_type type = static_cast<uhv::uh_file_type>(meta_data.type());

    if (type == uh::uhv::uh_file_type::regular)
    {
        stbuf->st_size = meta_data.size();
        stbuf->st_nlink = 1;
        stbuf->st_mode = S_IFREG | 0666;
    }

    if (type == uh::uhv::uh_file_type::directory)
    {
        stbuf->st_mode = S_IFDIR | 0766;
        stbuf->st_nlink = 2 + get_context()->subdirectory_counts() ().at(meta_data.path());
    }

    std::cout << "leaving uh_getattr(" << path << ", )\n";

    return 0;
}

int uh_unlink(const char *path)
{
    std::cout << "uh_unlink(" << path << ")\n";

    uh::uhv::uh_file_type type;

    auto *ctx = get_context();
    auto container_handle = ctx->metadata_map();
    auto& unordered_map = container_handle();
    auto it = unordered_map.find(path);
    if (it == unordered_map.end())
    {
        std::cout << "leaving uh_unlink(" << path << ")\n";
        THROW(exception, "file not found", ENOENT);
    }

    // !!! container should be released here since meta_data is found
    auto meta_handle = it->second.get();
    auto& meta_data = meta_handle();

    type = static_cast <uh::uhv::uh_file_type> (meta_data.type());

    if (type == uh::uhv::uh_file_type::regular)
    {
        unordered_map.erase(it);
    }

    ctx->update_uhv();

    std::cout << "leaving uh_unlink(" << path << ")\n";

    return 0;
}

int uh_rmdir (const char *path)
{
    std::cout << "uh_rmdir(" << path << ")\n";

    std::string pathString(path);

    if(pathString.ends_with("."))
    {
        THROW(exception, "invalid path name", EINVAL);
    }

    auto *ctx = get_context();
    auto unordered_map = ctx->metadata_map();
    auto it = unordered_map->find(pathString);
    if (it == unordered_map->end())
    {
        THROW(exception, "file not found", ENOENT);
    }

    // check if the d
    for(auto& [key, value] : *unordered_map)
    {
        if (key.starts_with(pathString) and key.compare(pathString))
        {
            THROW(exception, "directory not empty", ENOTEMPTY);
        }
    }

    auto meta_data = it->second.get();

    uh::uhv::uh_file_type type = static_cast<uh::uhv::uh_file_type>(meta_data->type());
    if(type == uh::uhv::uh_file_type::regular)
    {
        THROW(exception, "file is not a directory", ENOTDIR);
    }

    if(type == uh::uhv::uh_file_type::directory)
    {
        unordered_map->erase(it);
        ctx->subdirectory_counts()().erase(meta_data->path());
        ctx->subdirectory_counts()().at(meta_data->path().parent_path())--;
    }

    ctx->update_uhv();

    std::cout << "leaving uh_unlink(" << pathString << ")\n";
    return 0;
}

int __uh_ftruncate (const char *path, off_t off, struct fuse_file_info *fi)
{
    std::cout << "uh_ftruncate(" << path << ", " << off << ")\n";

    auto md = get_metadata(fi);

    if (md->type() != uh::uhv::uh_file_type::regular)
    {
        THROW(exception, "file is not a regular file", ENOMEM);
    }

    if (static_cast<size_t>(off) > md->size())
    {
        THROW(exception, "length is larget than the file size", EFBIG);
    }

    md->set_size(off);

    if (off == 0)
    {
        md->set_size(0);
        md->set_effective_size(0);
        md->set_hashes({});

        get_context()->update_uhv();
    }
    else
    {
        throw std::runtime_error("ftruncate operation not supported");
    }

    return 0;
}

int __uh_mkdir(const char *path, mode_t mode)
{
    std::cout << "uh_mkdir(" << path << ")\n";

    auto unordered_map = get_context()->metadata_map();

    // TODO: sanitize the path
    if (unordered_map->find(path) != unordered_map->end())
    {
        THROW(exception, "file not found", ENOENT);
    }

    // create meta data
    meta_data md;
    md.set_path(path);
    md.set_type(uh::uhv::directory);
    md.set_size(0u);
    md.set_permissions(static_cast<std::uint32_t>(mode));

    uh::uhv::file uhv(get_options().UHVpath);
    uhv.append(std::make_unique<uhv::meta_data>(md));

    unordered_map->emplace(std::string(path), md);
    get_context()->subdirectory_counts()().emplace(md.path(), 0);
    get_context()->subdirectory_counts()().at(md.path().parent_path())++;

    std::cout << "leaving uh_mkdir(" << path << ", )\n";

    return 0;
}

int __uh_readdir (const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
{
    std::cout << "uh_readdir(" << path << ", buf, filler, " << offset << ", fi)\n";

    (void) offset;
    (void) fi;

    std::vector <std::filesystem::path> files;
    {
        auto container_handle = get_context()->metadata_map();
        auto& unordered_map = container_handle();
        auto meta_handle = unordered_map.at(path).get();
        const auto& metadata = meta_handle();

        if (metadata.type() != uh::uhv::uh_file_type::directory)
        {
            THROW(exception, "file is not a directory", ENOENT);
        }

        files = get_files(metadata.path(), unordered_map);
    }

    filler(buf, ".", nullptr, 0);
    filler(buf, "..", nullptr, 0);

    for (const auto& file: files)
    {
        struct stat uh_stat {};
        uh_getattr(file.c_str(), &uh_stat);
        filler(buf, file.filename().c_str(), &uh_stat, 0);
    }

    std::cout << "leaving uh_readdir(" << path << ", )\n";
    return 0;
}

int __uh_open (const char *path, struct fuse_file_info *fi)
{
    std::cout << "open(" << path << ", fi)\n";

    if ((fi->flags & O_ACCMODE) != O_RDONLY and (fi->flags & O_ACCMODE) != O_RDWR and (fi->flags & O_ACCMODE) != O_WRONLY)
    {
        THROW(exception, "access denied", EACCES);
    }

    auto metadata_map = get_context()->metadata_map();
    auto it = metadata_map->find(path);
    if (it == metadata_map->end())
    {
        THROW(exception, "file not found", ENOENT);
    }

    set_metadata(fi, it->second);

    std::cout << "leaving uh_open(" << path << ", )\n";;
    return 0;
}

int __uh_release (const char *path, struct fuse_file_info *fi)
{
    if (fi->fh == 0)
    {
        THROW(exception, "bad file descriptor", EBADF);
    }

    fi->fh = 0;
    return 0;
}

int __uh_utimens (const char *path, const struct timespec tv[2]) {
    std::cout << "uh_utimens(" << path << ")\n";
    return 0;
}

int __uh_read (const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi)
{
    std::cout << "uh_read(" << path << ", " << size << ", " << offset << ")\n";

    if ((fi->flags & O_ACCMODE) != O_RDONLY and (fi->flags & O_ACCMODE) != O_RDWR)
    {
        THROW(exception, "access denied", EACCES);
    }

    auto md = get_metadata(fi);
    auto client = get_context()->get_client();

    size_t curr_offset = 0;
    std::stringstream recompiled_chunks;
    if (md->type() == uh::uhv::uh_file_type::regular)
    {
        std::vector<char> current_hash(64);

        for (auto i = 0u; i < md->hashes().size(); i += 64)
        {
            std::copy(md->hashes().begin() + i,
                      md->hashes().begin() + i + 64, current_hash.begin());

            auto resp = client->read_chunks({ .hashes = current_hash });
            auto& vector = std::get<0>(resp.data);
            memcpy(buffer + curr_offset, vector.data(), vector.size());
            curr_offset += vector.size();
        }
    }
    std::cout << "leaving uh_read(" << path << ", )\n";

    return size;
}

int __uh_write (const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    std::cout << "uh_write(" << path << ", " << size << ", " << offset << ")\n";

    if ((fi->flags & O_ACCMODE) != O_RDWR and (fi->flags & O_ACCMODE) != O_WRONLY)
    {
        THROW(exception, "access denied", EACCES);
    }

    auto md = get_metadata(fi);
    if (md->type() != uh::uhv::uh_file_type::regular)
    {
        THROW(exception, "file is not a regular file", ENOMEM);
    }

    auto context = get_context();
    auto client = context->get_client();

    if (static_cast<size_t>(offset) == md->size() and offset > 0)
    {
        // if this is an append, for instance when performing "echo data >> file"

        // read the last chunk of the file
        std::vector<char> current_hash(64);
        std::vector<char> last_chunk(std::max(md->size() % max_chunk_size, min_chuck_size) + size);
        std::span<char> data = {last_chunk.data(), last_chunk.size() - size};
        std::copy(md->hashes().end() - 64, md->hashes().end (), current_hash.begin());
        md->remove_hash(md->hashes().size() - 64, md->hashes().size());
        client->read_block(current_hash)->read(data);

        // append the new data to the last chunk
        std::copy(buf, buf + size, last_chunk.begin() + md->size() % max_chunk_size);
        data = {last_chunk.data(), md->size() % max_chunk_size + size};

        //write the extended last chunk
        const auto effective_size = upload_data(client, max_chunk_size, data, md->get_hashes());
        md->add_effective_size(effective_size);
        md->set_size(md->size() + size);
    }
    else if (offset == 0)
    {
        // for now, we assume replace data which is usually the case
        // since text editors write the whole file after modifications
        std::span<char> data = {const_cast <char*> (buf), size};
        md->set_hashes({});
        const auto effective_size = upload_data(client, max_chunk_size, data, md->get_hashes());
        md->set_effective_size(effective_size);
        md->set_size(size);
    }
    else
    {
        throw std::runtime_error("write operation not supported");
    }

    context->update_uhv();

    std::cout << "leaving uh_write(" << path << ", )\n";

    return size;
}

int __uh_create (const char *path, mode_t mode, struct fuse_file_info *fi) {
    std::cout << "uh_create(" << path << ")\n";

    if ((fi->flags & O_ACCMODE) != O_RDWR and (fi->flags & O_ACCMODE) != O_WRONLY)
    {
        THROW(exception, "access denied", EACCES);
    }

    uh::uhv::meta_data md(path);
    md.set_type(uh::uhv::uh_file_type::regular);
    md.set_size(0);
    md.set_effective_size(0);
    md.set_hashes({});
    md.set_permissions(mode);

    auto *context = get_context();
    auto files = context->metadata_map();

    auto res_pair = files->emplace(std::string(path), md);
    set_metadata(fi, res_pair.first->second);

    std::cout << "md appending: " << md.path() << "\n";
    context->uhv().append(std::make_unique<uhv::meta_data>(md));

    return 0;
}

void __uh_destroy (void *context) {
    std::cout << "uh_destroy(context)\n";

    delete static_cast<uh::fuse::context*>(context);
}

/*-----  fuse operations made safe -----*/

#define TRY_CATCH( EXPR )                                                       \
    try                                                                         \
    {                                                                           \
        EXPR;                                                                   \
    }                                                                           \
    catch (const exception& e)                                                  \
    {                                                                           \
        std::cerr << "Error in " << __FILE__ << ":" << __LINE__ << ": "         \
                  << e.what() << "\n";                                          \
        return -e.error();                                                      \
    }                                                                           \
    catch (const std::exception &e)                                             \
    {                                                                           \
        std::cerr << "Error in " << __FILE__ << ":" << __LINE__ << ": "         \
                  << e.what() << "\n";                                          \
        return -ENOENT;                                                         \
    }                                                                           \
    catch (...)                                                                 \
    {                                                                           \
        std::cerr << "Unknown error in " << __FILE__ << ":" << __LINE__ << "\n";\
        return -ENOENT;                                                         \
    }                                                                           \
    do {} while (false)

int uh_readdir (const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
{
    TRY_CATCH( return __uh_readdir(path, buf, filler, offset, fi) );
}

int uh_read (const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    TRY_CATCH( return __uh_read(path, buf, size, offset, fi) );
}

int uh_write (const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    TRY_CATCH( return __uh_write(path, buf, size, offset, fi) );
}

int uh_open (const char *path, struct fuse_file_info *fi)
{
    TRY_CATCH( return __uh_open(path, fi) );
}

int uh_release (const char *path, struct fuse_file_info *fi)
{
    TRY_CATCH( return __uh_release(path, fi) );
}

int uh_utimens (const char *path, const struct timespec tv[2])
{
    TRY_CATCH( return __uh_utimens (path, tv) );
}

int uh_getattr (const char *path, struct stat *stbuf)
{
    TRY_CATCH( return __uh_getattr(path, stbuf) );
}

int uh_ftruncate (const char *path, off_t off, struct fuse_file_info *fi)
{
    TRY_CATCH( return __uh_ftruncate(path, off, fi) );
}

int uh_create (const char *path, mode_t mode, struct fuse_file_info *fi)
{
    TRY_CATCH( return __uh_create(path, mode, fi) );
}

int uh_mkdir (const char *path, mode_t mode)
{
    TRY_CATCH( return __uh_mkdir (path, mode) );
}

void uh_destroy (void *context)
{
    try
    {
        return __uh_destroy(context);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error in " << __FILE__ << ":" << __LINE__
                  << e.what() << "\n";
    }
    catch (...)
    {
        std::cerr << "Unknown error in " << __FILE__ << ":" << __LINE__ << "\n";
    }
}

} // end namespace uh::uhv
