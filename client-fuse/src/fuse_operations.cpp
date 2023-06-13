//
// Created by masi on 23.03.23.
//
#include "fuse_operations.h"
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
        return -ENOENT;
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
        return ENOENT;
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

    uh::uhv::uh_file_type type;
    std::string pathString(path);

    if(pathString.ends_with("."))
    {
        std::cout << "leaving uh_unlink(" << pathString << ")\n";
        return -EINVAL;
    }

    auto *ctx = get_context();
    auto container_handle = ctx->metadata_map();
    auto& unordered_map = container_handle();
    auto it = unordered_map.find(pathString);
    if (it == unordered_map.end())
    {
        std::cout << "leaving uh_unlink(" << pathString << ")\n";
        return -ENOENT;
    }

    // check if the d
    for(auto& [key, value] : unordered_map) {
        // When a key starts with path but does not equal it, there are still sub-folders or files within that path
        if(key.starts_with(pathString) and key.compare(pathString)) {
            std::cout << "leaving uh_unlink(" << pathString << ")\n";
            return -ENOTEMPTY;
        }
    }

    // !!! container should be released here since memeta_handleta_data is found
    auto meta_handle = it->second.get();
    auto& meta_data = meta_handle();

    type = static_cast <uh::uhv::uh_file_type> (meta_data.type());

    if(type == uh::uhv::uh_file_type::regular)
    {
        std::cout << "leaving uh_unlink(" << pathString << ")\n";
        return -ENOTDIR;
    }
    else if(type == uh::uhv::uh_file_type::directory)
    {
        unordered_map.erase(it);
        ctx->subdirectory_counts()().erase(meta_data.path());
        ctx->subdirectory_counts()().at(meta_data.path().parent_path())--;
    }

    ctx->update_uhv();

    return 0;
}

int __uh_ftruncate (const char *path, off_t off, struct fuse_file_info *fi) {
    std::cout << "uh_ftruncate(" << path << ", " << off << ")\n";

    auto context = get_context();

    auto &ts_fmd = context->open_files()().at(fi->fh);
    auto fmd_handler = ts_fmd.get();
    auto &fmd = fmd_handler();

    if (fmd.type() != uh::uhv::uh_file_type::regular) {
        return -ENOMEM;
    }

    if (static_cast<size_t>(off) > fmd.size()) {
        return -EFBIG;
    }

    fmd.set_size(off);

    if (off == 0) {
        fmd.set_size(0);
        fmd.set_effective_size(0);
        fmd.set_hashes({});

        context->update_uhv();
    }
    else {  // for now we do nothing
        throw std::runtime_error("ftruncate operation not supported");
    }

    return 0;
}

int __uh_mkdir(const char *path, mode_t mode)
{
    std::cout << "uh_mkdir(" << path << ")\n";

    auto container_handle = get_context()->metadata_map();
    auto& unordered_map = container_handle();

    // TODO: sanitize the path
    if (unordered_map.find(path) != unordered_map.end())
        return -ENOENT;


    // create meta data
    meta_data md;
    md.set_path(path);
    md.set_type(uh::uhv::directory);
    md.set_size(0u);
    md.set_permissions(static_cast<std::uint32_t>(mode));

    uh::uhv::file uhv(get_options().UHVpath);
    uhv.append(std::make_unique<uhv::meta_data>(md));

    unordered_map.emplace(std::string(path), md);
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
            std::cout << "leaving uh_readdir(" << path << ", )\n";
            return -ENOENT;
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
        return -EACCES;
    }

    auto container_handle = get_context()->metadata_map();
    auto& metadata_map = container_handle();
    auto it = metadata_map.find(path);
    if (it == metadata_map.end())
    {
        return -EACCES;
    }
    else {
        auto open_files_map_handler = get_context()->open_files();
        open_files_map_handler().emplace(get_context()->fd, it->second);
        fi->fh = get_context()->fd;
        get_context()->fd ++;
    }

    std::cout << "leaving uh_open(" << path << ", )\n";;

    return 0;
}

int __uh_release (const char *path, struct fuse_file_info *fi) {
    auto open_files_handler = get_context()->open_files();
    auto &open_files_map = open_files_handler();
    if (const auto &it = open_files_map.find(fi->fh); it != open_files_map.end()) {
        // TODO: later we could reuse the released fd by adding it to a fd_queue. When opening a
        // file, if the fd_queue is not empty we pop one fd from the queue, otherwise, we increment context->fd;
        open_files_map.erase(it);
    }
    else {
        return -EBADF;
    }
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
        return -EACCES;
    }

    auto context = get_context();

    auto fmd_handler = context->open_files();
    auto fmd = fmd_handler().at(fi->fh).get() ();

    protocol::client_pool::handle client_handle = context->get_client();

    size_t curr_offset = 0;
    std::stringstream recompiled_chunks;
    if (fmd.type() == uh::uhv::uh_file_type::regular)
    {
        std::vector<char> current_hash(64);

        for (auto i = 0u; i < fmd.hashes().size(); i += 64)
        {
            std::copy(fmd.hashes().begin() + i,
                      fmd.hashes().begin() + i + 64, current_hash.begin());

            client_handle->read_block(current_hash)->read({buffer + curr_offset, 1024*1024});
            curr_offset += 1024*1024;
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
        return -EACCES;
    }

    auto context = get_context();
    auto& ts_fmd = context->open_files()().at(fi->fh);
    auto fmd_handler = ts_fmd.get();
    auto& fmd = fmd_handler();

    if (fmd.type() != uh::uhv::uh_file_type::regular)
    {
        return -ENOMEM;
    }

    protocol::client_pool::handle client_handle = context->get_client();

    if (static_cast<size_t>(offset) == fmd.size() and offset > 0)
    {       // if this is an append, for instance when performing "echo data >> file"

        // read the last chunk of the file
        std::vector<char> current_hash(64);
        std::vector<char> last_chunk(std::max(fmd.size() % max_chunk_size, min_chuck_size) + size);
        std::span<char> data = {last_chunk.data(), last_chunk.size() - size};
        std::copy(fmd.hashes().end() - 64, fmd.hashes().end (), current_hash.begin());
        fmd.remove_hash(fmd.hashes().size() - 64, fmd.hashes().size());
        client_handle->read_block(current_hash)->read(data);

        // append the new data to the last chunk
        std::copy(buf, buf + size, last_chunk.begin() + fmd.size() % max_chunk_size);
        data = {last_chunk.data(), fmd.size() % max_chunk_size + size};

        //write the extended last chunk
        const auto effective_size = upload_data(client_handle, max_chunk_size, data, fmd.get_hashes());
        fmd.add_effective_size(effective_size);
        fmd.set_size(fmd.size() + size);
    }
    else if (offset == 0) {         // for now, we assume replace data which is usually the case
                                    // since text editors write the whole file after modifications

        std::span<char> data = {const_cast <char*> (buf), size};
        fmd.set_hashes({});
        const auto effective_size = upload_data (client_handle, max_chunk_size, data, fmd.get_hashes());
        fmd.set_effective_size(effective_size);
        fmd.set_size(size);

    }
    else {
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
        return -EACCES;
    }

    uh::uhv::meta_data md(path);
    md.set_type(uh::uhv::uh_file_type::regular);
    md.set_size(0);
    md.set_effective_size(0);
    md.set_hashes({});
    md.set_permissions(mode);

    auto *context = get_context();
    auto container_handle = context->metadata_map();
    auto& files = container_handle();

    auto res_pair = files.emplace(std::string (path), md);
    auto meta_handle = res_pair.first->second.get();
    set_metadata(fi, meta_handle());

    uh::uhv::file uhv(get_options().UHVpath);
    uhv.append(std::make_unique<uhv::meta_data>(md));

    return 0;
}

void __uh_destroy (void *context) {
    std::cout << "uh_destroy(context)\n";

    delete static_cast<uh::fuse::context*>(context);
}

/*-----  fuse operations made safe -----*/

int uh_readdir (const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
{
    try
    {
       return __uh_readdir(path, buf, filler, offset, fi);
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return -ENOENT;
    }
}

int uh_read (const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    try
    {
        return __uh_read(path, buf, size, offset, fi);
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return -ENOENT;
    }
}

int uh_write (const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    try
    {
        return __uh_write (path, buf, size, offset, fi);
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return -ENOENT;
    }
}

int uh_open (const char *path, struct fuse_file_info *fi)
{
    try
    {
        return __uh_open(path, fi);
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return -ENOENT;
    }
}

int uh_release (const char *path, struct fuse_file_info *fi)
{
    try
    {
        return __uh_release(path, fi);
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return -ENOENT;
    }
}

int uh_utimens (const char *path, const struct timespec tv[2])
{
    try
    {
        return __uh_utimens (path, tv);
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return -ENOENT;
    }
}

int uh_getattr (const char *path, struct stat *stbuf)
{
    try
    {
        return __uh_getattr(path, stbuf);
    }
    catch(const std::exception& e)
    {
        std::cerr << "uh_getattr error: " << e.what() << '\n';
        return -ENOENT;
    }
}

int uh_ftruncate (const char *path, off_t off, struct fuse_file_info *fi) {
    try
    {
        return __uh_ftruncate(path, off, fi);
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return -ENOENT;
    }
}

int uh_create (const char *path, mode_t mode, struct fuse_file_info *fi) {
    try
    {
        return __uh_create(path, mode, fi);
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return -ENOENT;
    }
}


int uh_mkdir (const char *path, mode_t mode) {
    try
    {
        return __uh_mkdir (path, mode);
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return -ENOENT;
    }
}

void uh_destroy (void *context)
{
    try
    {
        return __uh_destroy(context);
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
}

} // end namespace uh::uhv
