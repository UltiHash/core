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
    auto container_handle = ctx->container.get();
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
        stbuf->st_nlink = 2 + subfolders_count (path, unordered_map);
    }
    std::cout << "leaving uh_getattr(" << path << ", )\n";

    return 0;
}

int uh_unlink(const char *path)
{
    std::cout << "uh_unlink(" << path << ")\n";

    uh::uhv::uh_file_type f_type;

    auto *ctx = get_context();
    auto container_handle = ctx->container.get();
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
    auto container_handle = ctx->container.get();
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

    auto container_handle = context->container.get();
    auto& unordered_map = container_handle();

    while (const auto& metadata = metadata_list.get_job())
    {
        if (metadata == std::nullopt)
            break;
        unordered_map.emplace(metadata.value()->f_path(), *(*metadata));
    }
    f_meta_data metadata;
    metadata.set_f_path("/");
    metadata.set_f_type(uh::uhv::directory);
    metadata.set_f_size(0u);
    unordered_map.emplace("/", std::move (metadata));

    return context;
}

int __uh_ftruncate (const char *path, off_t off, struct fuse_file_info *fi) {
    std::cout << "uh_ftruncate(" << path << ", " << off << ")\n";

    auto &fmd = *reinterpret_cast<uh::uhv::f_meta_data*>(fi->fh);
    if (fmd.f_type() != uh::uhv::uh_file_type::regular) {
        return -ENOMEM;
    }

    if (off > fmd.f_size()) {
        return -EFBIG;
    }

    fmd.set_f_size(off);

    if (off == 0) {
        fmd.set_f_size(0);
        fmd.set_effective_size(0);
        fmd.set_f_hashes({});

        // store the new metadata into the uh volume
        auto container = get_context()->container.get();
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

    auto container_handle = get_context()->container.get();
    auto& unordered_map = container_handle();

    // TODO: sanitize the path
    if (unordered_map.find(path) != unordered_map.end())
        return -ENOENT;


    // create meta data
    std::unique_ptr<uh::uhv::f_meta_data> ptr_f_meta_data = std::make_unique<uh::uhv::f_meta_data>();
    ptr_f_meta_data->set_f_path(path);
    ptr_f_meta_data->set_f_type(uh::uhv::directory);
    ptr_f_meta_data->set_f_size(0u);
    ptr_f_meta_data->set_f_permissions(static_cast<std::uint32_t>(mode));

    // store the new metadata into the uh volume
    std::ofstream UHV_file(get_options().UHVpath, std::ios::app | std::ios::out | std::ios::binary);
    write_metadata(UHV_file, *ptr_f_meta_data);


    unordered_map.emplace(std::string(path), std::move (*ptr_f_meta_data));

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
        auto container_handle = get_context()->container.get();
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

    auto container_handle = get_context()->container.get();
    auto& unordered_map = container_handle();
    auto it = unordered_map.find(path);
    if (it == unordered_map.end())
    {
        return -EACCES;
    }
    else {
        auto meta_handle = it->second.get();
        auto& meta_data = meta_handle();
        fi->fh = reinterpret_cast<uint64_t>(&meta_data);
    }

    std::cout << "leaving uh_open(" << path << ", )\n";;

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
    uh::protocol::client_pool::handle client_handle = context->client_pool->get();

    auto &fmd = *reinterpret_cast<uh::uhv::f_meta_data*>(fi->fh);

    size_t curr_offset = 0;
    std::stringstream recompiled_chunks;
    if (fmd.f_type() == uh::uhv::uh_file_type::regular)
    {
        std::vector<char> current_hash(64);

        for (auto i = 0; i < fmd.f_hashes().size(); i += 64)
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

uh::uhv::f_meta_data& get_fmetadata(struct fuse_file_info *fi){
  if(fi->fh == 0){
//    THROW(...)
    throw std::runtime_error("error while getting metadata");
  }
    return *reinterpret_cast<uh::uhv::f_meta_data*>(fi->fh);
}

int __uh_write (const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {

    std::cout << "uh_write(" << path << ", " << size << ", " << offset << ")\n";

    if ((fi->flags & O_ACCMODE) != O_RDWR and (fi->flags & O_ACCMODE) != O_WRONLY)
    {
        return -EACCES;
    }
    auto &fmd = get_fmetadata(fi);
    if (fmd.f_type() != uh::uhv::uh_file_type::regular) {
        return -ENOMEM;
    }

    auto context = get_context();
    uh::protocol::client_pool::handle&& client_handle = context->client_pool->get();

    if (offset == fmd.f_size() and offset > 0) {       // if this is an append, for instance when performing "echo data >> file"

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
    auto container = get_context()->container.get();
    auto &handler = container();
    rewrite_uhv_file(get_options().UHVpath, handler);

    std::cout << "leaving uh_write(" << path << ", )\n";

    return size;
}

int __uh_write_buf (const char *path, struct fuse_bufvec *buf_vec, off_t off, struct fuse_file_info *fi){
    int success = - ENOENT;

    size_t size = buf_vec->buf->size;
    size_t offset = buf_vec->off;
    char * buf = static_cast<char*>(buf_vec->buf->mem);
    __uh_write(path, buf, size, offset, fi);
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
    auto container_handle = context->container.get();
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


int __uh_read_buf (const char *path, struct fuse_bufvec **bufp, size_t size, off_t off, struct fuse_file_info *fi){
        throw std::runtime_error("Not implemented");
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

int uh_write_buf (const char *path, struct fuse_bufvec *buf, off_t off, struct fuse_file_info *fi){
    try
    {
        return __uh_write_buf(path, buf, off, fi);
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return -ENOENT;
    }
}

int uh_read_buf (const char *path, struct fuse_bufvec **bufp, size_t size, off_t off, struct fuse_file_info *fi){
    try
    {
        return __uh_read_buf(path, bufp, size, off, fi);
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return -ENOENT;
    }
}

} // end namespace uh::uhv
