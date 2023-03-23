/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

  This program can be distributed under the terms of the GNU GPLv2.
  See the file COPYING.
*/

#define FUSE_USE_VERSION 31

#include "config.hpp"
#include <iostream>
#include <fuse.h>
#include <cstring>
#include <filesystem>
#include <logging/logging_boost.h>
#include <unordered_map>
#include <uhv/job_queue.h>
#include <uhv/f_serialization.h>
#include <uhv/f_meta_data.h>
#include <protocol/client_factory.h>
#include <protocol/client_pool.h>
#include <net/plain_socket.h>
#include <util/exception.h>

using namespace uh::uhv;

#define OPTION(t, p)                           \
    { t, offsetof(struct options, p), 1 }

static struct options
{
    const char *UHVpath;
    const char *agency_hostname;
    int agency_port;
    int agency_connections;
    bool show_help;
} options;

struct private_context
{
    std::unique_ptr<uh::protocol::client_pool> client_pool;
    std::unordered_map <std::string, uh::uhv::f_meta_data> paths_metadata;
};

private_context* get_context()
{
    return static_cast<private_context*>(fuse_get_context()->private_data);
}

f_meta_data* get_metadata(struct fuse_file_info* fi)
{
    return reinterpret_cast<f_meta_data*>(fi->fh);
}

#define OPTION(t, p)                           \
    { t, offsetof(struct options, p), 1 }
static const struct fuse_opt option_spec[] =
    {
        OPTION("--path=%s", UHVpath),
        OPTION("-p=%s", UHVpath),
        OPTION("--agency-hostname=%s", agency_hostname),
        OPTION("-H=%s", agency_hostname),
        OPTION("--agency-port=%i", agency_port),
        OPTION("-P=%i", agency_port),
        OPTION("--agency-connections=%i", agency_connections),
        OPTION("-C=%i", agency_connections),
        OPTION("--help", show_help),
        OPTION("-h", show_help),
        FUSE_OPT_END
    };

int uh_getattr (const char *path, struct stat *)
{
    return 0;
}

void *uh_init (struct fuse_conn_info *conn)
{

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
                        io, options.agency_hostname, options.agency_port),
                cf_config), options.agency_connections));

    uh::uhv::job_queue<std::unique_ptr<uh::uhv::f_meta_data>> metadata_list;
    uh::uhv::f_serialization serializer {std::filesystem::path (options.UHVpath), metadata_list};
    serializer.deserialize("", false);
    while (const auto& metadata = metadata_list.get_job())
    {
        if (metadata == std::nullopt)
            break;
        context->paths_metadata.emplace(metadata.value()->f_path(), *(*metadata));
    }
    return context;
}

std::vector <std::string> get_files (const std::string &directory, const std::unordered_map <std::string, uh::uhv::f_meta_data> &metadata_list) {
    std::vector <std::string> files;
    for (const auto& md: metadata_list) {
        if (std::filesystem::path (md.first).parent_path() == directory) {
            files.emplace_back (md.first);
        }
    }
    return files;
}

int uh_readdir (const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
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

int uh_open (const char* path, struct fuse_file_info* fi)
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

int uh_read (const char *, char *, size_t, off_t, struct fuse_file_info *)
{
    return 0;
}

void uh_destroy (void *context) {
    auto pcontext = static_cast <private_context *> (context);
    delete pcontext;
}


static const struct fuse_operations uh_operations =
    {
        .getattr        = uh_getattr,
        .open           = uh_open,
        .read           = uh_read,
        .readdir        = uh_readdir,
        .init           = uh_init,
        .destroy        = uh_destroy,
    };

static void show_help(const char *prog_name)
{
    printf("\nusage: %s <mountpoint> [options]\n\n", prog_name);
    printf("File-system specific options:\n"
           "    -p or --path=<s>                    Path of the \"UltiHash\" Volume\n"
           "    -a or --agency-hostname=<s>         Contents \"hello\" file\n"
           "\n");
}

void validate_options()
{
    canonical(std::filesystem::path(options.UHVpath));
    if(options.agency_port < 0 or options.agency_port > USHRT_MAX)
        THROW(uh::util::exception, "An invalid port number was specified.");
    if(options.agency_connections < 0 )
        THROW(uh::util::exception, "An invalid number of connections was specified.");
}

int main(int argc, char *argv[])
{
    try
    {
        int ret = 0;
        struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

        /* Default Values */
        options.UHVpath = strdup("volume.uh");
        options.agency_hostname = strdup("localhost");
        options.agency_port = 21832;
        options.agency_connections = 3;
        options.show_help = false;

        /* Parse options */
        if (fuse_opt_parse(&args, &options, option_spec, NULL) == -1)
            throw std::runtime_error("error: parsing failed");

        if (options.show_help)
        {
            show_help(argv[0]);
            args.argv[0][0] = '\0';
        }
        else
        {
            validate_options();
            ret = fuse_main(args.argc, args.argv, &uh_operations, NULL);
        }

        fuse_opt_free_args(&args);
        return ret;
    }
    catch (const std::exception& exc)
    {
        FATAL << exc.what();
        return EXIT_FAILURE;
    }
    catch (...)
    {
        FATAL << "unknown exception occurred";
        return EXIT_FAILURE;
    }
}

// ---------------------------------------------------------------------
