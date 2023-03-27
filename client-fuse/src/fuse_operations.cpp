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
    if (it == unordered_map.end())
    {
        std::cout << "leaving uh_getattr(" << path << ", )\n";
        return 0;
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
        stbuf->st_nlink = 2;
    }
    std::cout << "leaving uh_getattr(" << path << ", )\n";

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

int truncate (const char *path, off_t off) {
    std::cout << "uh_truncate(" << path << ", " << off << ")\n";
    return 0;
}

int ftruncate (const char *path, off_t off, struct fuse_file_info *fi) {
    std::cout << "uh_ftruncate(" << path << ", " << off << ")\n";
    return 0;
}



int __uh_readdir (const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
{
    std::cout << "uh_readdir(" << path << ", buf, filler, " << offset << ", fi)\n";

    (void) offset;
    (void) fi;

    std::vector <std::string> files;
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
        filler(buf, file.c_str(), &uh_stat, 0);
    }
    std::cout << "leaving uh_readdir(" << path << ", )\n";

    return 0;
}

int __uh_open (const char *path, struct fuse_file_info *fi)
{

    auto context = get_context();
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
        return -ENOENT;
    }

    auto meta_handle = it->second.get();
    auto& meta_data = meta_handle();
    fi->fh = reinterpret_cast<uint64_t>(&meta_data);
    std::cout << "leaving uh_open(" << path << ", )\n";;

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

int __uh_write (const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {

    std::cout << "uh_write(" << path << ", " << size << ", " << offset << ")\n";

    if ((fi->flags & O_ACCMODE) != O_RDWR and (fi->flags & O_ACCMODE) != O_WRONLY)
    {
        return -EACCES;
    }
    auto &fmd = *reinterpret_cast<uh::uhv::f_meta_data*>(fi->fh);
    if (fmd.f_type() != uh::uhv::uh_file_type::regular) {
        return -ENOMEM;
    }

    auto context = get_context();
    uh::protocol::client_pool::handle&& client_handle = context->client_pool->get();

    constexpr std::uint64_t max_chunk_size = 1 << 22;
    constexpr std::uint64_t min_chuck_size = 64 * 1024ul;

    if (offset == fmd.f_size()) {       // if this is an append, for instance when performing "echo data >> file"

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
    std::ofstream UHV_file(get_options().UHVpath, std::ios::trunc | std::ios::out | std::ios::in | std::ios::binary);

    for (auto &tsmd: context->container.get()()) {
        auto &md = tsmd.second.get()();
        auto relative_path = (md.f_path() == "/") ? "/":std::filesystem::relative(md.f_path(), "/");
        auto bytes = uh::uhv::f_serialization::serialize_f_meta_data(std::make_unique <uh::uhv::f_meta_data> (md), relative_path);
        UHV_file.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    }
    UHV_file.flush();

    std::cout << "leaving uh_write(" << path << ", )\n";

    return size;
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
