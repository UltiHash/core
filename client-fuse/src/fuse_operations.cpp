//
// Created by masi on 23.03.23.
//
#include "fuse_operations.h"
#include "utils.h"

namespace uh::uhv {

options& get_options()
{
    static options opt;

    return opt;
}


/* --- fuse_operations core functionality --- */

int __uh_getattr (const char *path, struct stat *stbuf)
{
    std::cout << "uh_getattr(" << path << ", stbuf)\n";
    memset(stbuf, 0, sizeof(struct stat));
    uh::uhv::uh_file_type f_type;

    auto *ctx = get_context();
    auto container_handle = ctx->fmetadata_map.get();
    auto& unordered_map = container_handle();

    auto it = unordered_map.find(path);
    if (it == unordered_map.end() and (std::filesystem::path(path).filename().c_str()[0] == '.' or (strcmp (path,"/autorun.inf") == 0)))
    {
        std::cout << "leaving uh_getattr(" << path << ", )\n";
        return 0;
    }
    else if (it == unordered_map.end()) {
        //stat (path, stbuf);
        stbuf->st_size = 0;
        stbuf->st_nlink = 1;
        stbuf->st_blksize = 4096;
        stbuf->st_blocks = 0;
        clock_gettime(CLOCK_MONOTONIC, &stbuf->st_ctim);
        stbuf->st_mode = S_IFREG | 0666;
        return -ENOENT;
    }

    // !!! container should be released here since f_mera_data is found
    auto meta_handle = it->second.get();
    auto& f_meta_data = meta_handle();

    f_type = static_cast <uh::uhv::uh_file_type> (f_meta_data.f_type());

    if (f_type == uh::uhv::uh_file_type::regular)
    {
        stbuf->st_size = f_meta_data.f_size();
        stbuf->st_nlink = 1;
        stbuf->st_mode = S_IFREG | 0666;
    }
    if (f_type == uh::uhv::uh_file_type::directory)
    {
        stbuf->st_mode = S_IFDIR | 0766;
        stbuf->st_nlink = 2 + get_context()->subdirectory_counts.get() ().at(f_meta_data.f_path());
    }
    std::cout << "leaving uh_getattr(" << path << ", )\n";

    return 0;
}

int uh_unlink(const char *path)
{
    std::cout << "uh_unlink(" << path << ")\n";

    uh::uhv::uh_file_type f_type;

    auto *ctx = get_context();
    auto container_handle = ctx->fmetadata_map.get();
    auto& unordered_map = container_handle();
    auto it = unordered_map.find(path);
    if (it == unordered_map.end())
    {
        std::cout << "leaving uh_unlink(" << path << ")\n";
        return ENOENT;
    }

    // !!! container should be released here since f_meta_data is found
    auto meta_handle = it->second.get();
    auto& f_meta_data = meta_handle();

    f_type = static_cast <uh::uhv::uh_file_type> (f_meta_data.f_type());

    if (f_type == uh::uhv::uh_file_type::regular)
    {
        unordered_map.erase(it);
    }

    // store the new metadata into the uh volume
    rewrite_uhv_file(get_options().UHVpath, unordered_map);

    std::cout << "leaving uh_unlink(" << path << ")\n";

    return 0;
}

int uh_rmdir (const char *path)
{
    std::cout << "uh_rmdir(" << path << ")\n";

    uh::uhv::uh_file_type f_type;
    std::string pathString(path);

    if(pathString.ends_with("."))
    {
        std::cout << "leaving uh_unlink(" << pathString << ")\n";
        return -EINVAL;
    }

    auto *ctx = get_context();
    auto container_handle = ctx->fmetadata_map.get();
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

    // !!! container should be released here since f_memeta_handleta_data is found
    auto meta_handle = it->second.get();
    auto& f_meta_data = meta_handle();

    f_type = static_cast <uh::uhv::uh_file_type> (f_meta_data.f_type());

    if(f_type == uh::uhv::uh_file_type::regular)
    {
        std::cout << "leaving uh_unlink(" << pathString << ")\n";
        return -ENOTDIR;
    }
    else if(f_type == uh::uhv::uh_file_type::directory)
    {
        unordered_map.erase(it);
        ctx->subdirectory_counts.get()().erase(f_meta_data.f_path());
        ctx->subdirectory_counts.get()().at(f_meta_data.f_path().parent_path())--;
    }

    // store the new metadata into the uh volume
    rewrite_uhv_file(get_options().UHVpath, unordered_map);

    return 0;
}

void *__uh_init (struct fuse_conn_info *conn)
{
    std::cout << "uh_init(conn)\n";
    auto *context = new private_context;

    // protocol
    std::stringstream s;
    s << PROJECT_NAME << " " << PROJECT_VERSION;

    uh::protocol::client_factory_config cf_config
            {
                    .client_version = s.str()
            };
    context->client_pool = std::move(std::make_unique<uh::protocol::client_pool>(
        std::make_unique<uh::protocol::client_factory>(
                std::make_unique<uh::net::plain_socket_factory>(
                        context->io, get_options().agency_hostname, get_options().agency_port),
                cf_config), get_options().agency_connections));

    uh::uhv::job_queue<std::unique_ptr<uh::uhv::f_meta_data>> metadata_list;
    uh::uhv::f_serialization serializer {std::filesystem::path (get_options().UHVpath), metadata_list};

    serializer.deserialize("", false);
    metadata_list.stop();

    auto container_handle = context->fmetadata_map.get();
    auto& metadata_map = container_handle();

    auto subdirectories_handle = context->subdirectory_counts.get();
    auto &subdirectory_counts = subdirectories_handle();

    while (const auto& metadata = metadata_list.get_job())
    {
        if (metadata == std::nullopt)
            break;

        if (metadata->get()->f_type() == uh::uhv::uh_file_type::directory) {
            auto parent = metadata->get()->f_path().parent_path();
            if (const auto &it = subdirectory_counts.find(parent); it != subdirectory_counts.end()) {
                it->second ++;
            }
            else {
                subdirectory_counts.emplace(parent, 1);
            }
            subdirectory_counts.emplace(metadata->get()->f_path(), 0);
        }
        metadata_map.emplace(metadata.value()->f_path(), *(*metadata));
    }


    f_meta_data metadata;
    metadata.set_f_path("/");
    metadata.set_f_type(uh::uhv::directory);
    metadata.set_f_size(0u);
    metadata_map.emplace("/", std::move (metadata));

    return context;
}

int __uh_ftruncate (const char *path, off_t off, struct fuse_file_info *fi) {
    std::cout << "uh_ftruncate(" << path << ", " << off << ")\n";

    auto context = get_context();

    auto &ts_fmd = context->open_files.get()().at(fi->fh);
    auto fmd_handler = ts_fmd.get();
    auto &fmd = fmd_handler();

    if (fmd.f_type() != uh::uhv::uh_file_type::regular) {
        return -ENOMEM;
    }

    if (static_cast<size_t>(off) > fmd.f_size()) {
        return -EFBIG;
    }

    fmd.set_f_size(off);

    if (off == 0) {
        fmd.set_f_size(0);
        fmd.set_effective_size(0);
        fmd.set_f_hashes({});

        // store the new metadata into the uh volume
        auto container = context->fmetadata_map.get();
        auto &handler = container();
        rewrite_uhv_file(get_options().UHVpath, handler);

    }
    else {  // for now we do nothing
        throw std::runtime_error("ftruncate operation not supported");
    }

    return 0;
}

int __uh_mkdir(const char *path, mode_t mode)
{
    std::cout << "uh_mkdir(" << path << ")\n";

    auto container_handle = get_context()->fmetadata_map.get();
    auto& unordered_map = container_handle();

    // TODO: sanitize the path
    if (unordered_map.find(path) != unordered_map.end())
        return -ENOENT;


    // create meta data
    f_meta_data f_meta_data;
    f_meta_data.set_f_path(path);
    f_meta_data.set_f_type(uh::uhv::directory);
    f_meta_data.set_f_size(0u);
    f_meta_data.set_f_permissions(static_cast<std::uint32_t>(mode));

    // store the new metadata into the uh volume
    std::ofstream UHV_file(get_options().UHVpath, std::ios::app | std::ios::out | std::ios::binary);
    write_metadata(UHV_file, f_meta_data);


    unordered_map.emplace(std::string(path), f_meta_data);
    get_context()->subdirectory_counts.get()().emplace(f_meta_data.f_path(), 0);
    get_context()->subdirectory_counts.get()().at(f_meta_data.f_path().parent_path())++;

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
        auto container_handle = get_context()->fmetadata_map.get();
        auto& unordered_map = container_handle();
        auto meta_handle = unordered_map.at(path).get();
        const auto& metadata = meta_handle();

        if (metadata.f_type() != uh::uhv::uh_file_type::directory)
        {
            std::cout << "leaving uh_readdir(" << path << ", )\n";
            return -ENOENT;
        }

        files = get_files (metadata.f_path(), unordered_map);
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

    auto container_handle = get_context()->fmetadata_map.get();
    auto& metadata_map = container_handle();
    auto it = metadata_map.find(path);
    if (it == metadata_map.end())
    {
        return -EACCES;
    }
    else {
        auto open_files_map_handler = get_context()->open_files.get();
        open_files_map_handler().emplace(get_context()->fd, it->second);
        fi->fh = get_context()->fd;
        get_context()->fd ++;
    }

    std::cout << "leaving uh_open(" << path << ", )\n";;

    return 0;
}

int __uh_release (const char *path, struct fuse_file_info *fi) {
    auto open_files_handler = get_context()->open_files.get();
    auto &open_files_map = open_files_handler ();
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

    auto fmd_handler = context->open_files.get();
    auto fmd = fmd_handler().at(fi->fh).get() ();

    uh::protocol::client_pool::handle client_handle = context->client_pool->get();


    size_t curr_offset = 0;
    std::stringstream recompiled_chunks;
    if (fmd.f_type() == uh::uhv::uh_file_type::regular)
    {
        std::vector<char> current_hash(64);

        for (auto i = 0u; i < fmd.f_hashes().size(); i += 64)
        {
            std::copy(fmd.f_hashes().begin() + i,
                      fmd.f_hashes().begin() + i + 64, current_hash.begin());

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
    auto &ts_fmd = context->open_files.get()().at(fi->fh);
    auto fmd_handler = ts_fmd.get();
    auto &fmd = fmd_handler();

    if (fmd.f_type() != uh::uhv::uh_file_type::regular) {
        return -ENOMEM;
    }

    uh::protocol::client_pool::handle&& client_handle = context->client_pool->get();

    if (static_cast<size_t>(offset) == fmd.f_size() and offset > 0) {       // if this is an append, for instance when performing "echo data >> file"

        // read the last chunk of the file
        std::vector<char> current_hash(64);
        std::vector <char> last_chunk (std::max (fmd.f_size() % max_chunk_size, min_chuck_size) + size);
        std::span<char> data = {last_chunk.data(), last_chunk.size() - size};
        std::copy(fmd.f_hashes().end() - 64, fmd.f_hashes().end (), current_hash.begin());
        fmd.remove_hash(fmd.f_hashes().size() - 64, fmd.f_hashes().size());
        client_handle->read_block(current_hash)->read(data);

        // append the new data to the last chunk
        std::copy(buf, buf + size, last_chunk.begin() + fmd.f_size() % max_chunk_size);
        data = {last_chunk.data(), fmd.f_size() % max_chunk_size + size};

        //write the extended last chunk
        const auto effective_size = upload_data (client_handle, max_chunk_size, data, fmd.get_hashes());
        fmd.add_effective_size(effective_size);
        fmd.set_f_size(fmd.f_size() + size);
    }
    else if (offset == 0) {         // for now, we assume replace data which is usually the case
                                    // since text editors write the whole file after modifications

        std::span<char> data = {const_cast <char*> (buf), size};
        fmd.set_f_hashes({});
        const auto effective_size = upload_data (client_handle, max_chunk_size, data, fmd.get_hashes());
        fmd.set_effective_size(effective_size);
        fmd.set_f_size(size);

    }
    else {
        throw std::runtime_error("write operation not supported");
    }


    // store the new metadata into the uh volume
    auto container = get_context()->fmetadata_map.get();
    auto &handler = container();
    rewrite_uhv_file(get_options().UHVpath, handler);

    std::cout << "leaving uh_write(" << path << ", )\n";

    return size;
}

int __uh_create (const char *path, mode_t mode, struct fuse_file_info *fi) {
    std::cout << "uh_create(" << path << ")\n";

    if ((fi->flags & O_ACCMODE) != O_RDWR and (fi->flags & O_ACCMODE) != O_WRONLY)
    {
        return -EACCES;
    }

    uh::uhv::f_meta_data md (path);
    md.set_f_type(uh::uhv::uh_file_type::regular);
    md.set_f_size(0);
    md.set_effective_size(0);
    md.set_f_hashes({});
    md.set_f_permissions(mode);

    auto *context = get_context();
    auto container_handle = context->fmetadata_map.get();
    auto& files = container_handle();

    auto res_pair = files.emplace(std::string (path), md);
    auto meta_handle = res_pair.first->second.get();
    set_metadata(fi, meta_handle());

    // store the new metadata into the uh volume
    std::ofstream UHV_file(get_options().UHVpath, std::ios::app | std::ios::out | std::ios::binary);
    write_metadata(UHV_file, md);

    return 0;
}

void __uh_destroy (void *context) {
    std::cout << "uh_destroy(context)\n";

    auto pcontext = static_cast <private_context *> (context);
    delete pcontext;
}

/*-----  fuse operations made safe -----*/

void *uh_init (struct fuse_conn_info *conn){
    try
    {
        return __uh_init(conn);
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return nullptr;
    }
}

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
        std::cerr << e.what() << '\n';
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
