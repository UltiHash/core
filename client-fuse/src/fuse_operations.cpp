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

    auto it = get_context()->paths_metadata.find(path);
    if (it == get_context()->paths_metadata.end())
    {
        return 0;
    }

    auto f_meta_data = it->second;
    f_type = static_cast <uh::uhv::uh_file_type> (f_meta_data.f_type());

    if (f_type == uh::uhv::uh_file_type::regular)
    {
        stbuf->st_size = f_meta_data.f_size();
        stbuf->st_nlink = 1;
	    stbuf->st_mode = S_IFREG | 0444;
    }
    if (f_type == uh::uhv::uh_file_type::directory)
    {
	    stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
    }

    return 0;
}

void *__uh_init (struct fuse_conn_info *conn)
{
    std::cout << "uh_init(conn)\n";
    auto *context = new private_context;

    // protocol
    boost::asio::io_context io;
    std::stringstream s;
    s << PROJECT_NAME << " " << PROJECT_VERSION;

    uh::protocol::client_factory_config cf_config
            {
                    .client_version = s.str()
            };
    context->client_pool = std::move(std::make_unique<uh::protocol::client_pool>(
        std::make_unique<uh::protocol::client_factory>(
                std::make_unique<uh::net::plain_socket_factory>(
                        io, get_options().agency_hostname, get_options().agency_port),
                cf_config), get_options().agency_connections));

    uh::uhv::job_queue<std::unique_ptr<uh::uhv::f_meta_data>> metadata_list;
    uh::uhv::f_serialization serializer {std::filesystem::path (get_options().UHVpath), metadata_list};

    serializer.deserialize("", false);
    metadata_list.stop();
    while (const auto& metadata = metadata_list.get_job())
    {
        if (metadata == std::nullopt)
            break;
        context->paths_metadata.emplace(metadata.value()->f_path(), *(*metadata));
    }
    f_meta_data metadata;
    metadata.set_f_path("/");
    metadata.set_f_type(uh::uhv::directory);
    metadata.set_f_size(0u);
    context->paths_metadata["/"] = metadata;

    std::cout << "leaving uh_init(conn)\n";

    return context;
}



int __uh_readdir (const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
{
    (void) offset;
    (void) fi;

    const auto *fuse_context = fuse_get_context ();

    const auto *pcontext = static_cast <private_context *> (fuse_context->private_data);

    const auto metadata = pcontext->paths_metadata.at(path);
    if (metadata.f_type() != uh::uhv::uh_file_type::directory) {
        return -ENOENT;
    }

    filler(buf, ".", nullptr, 0);
    filler(buf, "..", nullptr, 0);

    for (const auto& file: get_files (metadata.f_path(), pcontext->paths_metadata)) {
        struct stat uh_stat {};
        uh_getattr(file.c_str(), &uh_stat);
        filler(buf, file.c_str(), &uh_stat, 0);
    }
    return 0;
}

int __uh_open (const char *path, struct fuse_file_info *fi)
{
    auto context = get_context();
    std::cout << "open(" << path << ", )\n";

    if ((fi->flags & O_ACCMODE) != O_RDONLY)
    {
        return -EACCES;
    }

    auto it = context->paths_metadata.find(path);
    if (it == context->paths_metadata.end())
    {
        return -ENOENT;
    }

    fi->fh = reinterpret_cast<uint64_t>(&it->second);
    return 0;
}

int __uh_read (const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *ffi)
{

    std::cout << "uh_read(" << path << ", )\n";
    auto context = get_context();
    uh::protocol::client_pool::handle&& client_handle = context->client_pool->get();
    auto* fmd = reinterpret_cast<uh::uhv::f_meta_data*>(ffi->fh);
    size_t curr_offset = 0;
    std::stringstream recompiled_chunks;
    if (fmd->f_type() == uh::uhv::uh_file_type::regular)
    {
        std::vector<char> current_hash(64);

        for (auto i = 0; i < fmd->f_hashes().size(); i += 64)
        {
            std::copy(fmd->f_hashes().begin() + i,
                      fmd->f_hashes().begin() + i + 64, current_hash.begin());


            recompiled_chunks << *client_handle->read_block(current_hash);
        }
    }
    std::string contents_read = recompiled_chunks.str();

    size = std::min(size, contents_read.size() - offset);
    memcpy(buffer, contents_read.c_str() + offset, size);

    return size;
}

void __uh_destroy (void *context) {
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

void uh_destroy (void *context){
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
