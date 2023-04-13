/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

  This program can be distributed under the terms of the GNU GPLv2.
  See the file COPYING.
*/

#define FUSE_USE_VERSION 31

#include "fuse_operations.h"


static const struct fuse_operations uh_operations =
    {
    .getattr        = uh::uhv::uh_getattr,
    .mkdir          = uh::uhv::uh_mkdir,
    .unlink         = uh::uhv::uh_unlink,
    .rmdir          = uh::uhv::uh_rmdir,
    .open           = uh::uhv::uh_open,
    .read           = uh::uhv::uh_read,
    .write          = uh::uhv::uh_write,
    .readdir        = uh::uhv::uh_readdir,
    .init           = uh::uhv::uh_init,
    .destroy        = uh::uhv::uh_destroy,
    .create         = uh::uhv::uh_create,
    .ftruncate      = uh::uhv::uh_ftruncate,
    .utimens        = uh::uhv::uh_utimens,
    };


#define OPTION(t, p)                           \
    { t, offsetof(struct uh::uhv::options, p), 1 }

static const struct fuse_opt option_spec[] =
    {
        OPTION("--path %s", UHVpath),
        OPTION("-p %s", UHVpath),
        OPTION("--agency-hostname %s", agency_hostname),
        OPTION("-H %s", agency_hostname),
        OPTION("--agency-port %i", agency_port),
        OPTION("-P %i", agency_port),
        OPTION("--agency-connections %i", agency_connections),
        OPTION("-C %i", agency_connections),
        OPTION("--help", show_help),
        OPTION("-h", show_help),
        FUSE_OPT_END
    };

static void show_help(const char *prog_name)
{
    printf("\nusage: %s <mountpoint> [options]\n\n", prog_name);
    printf("File-system specific options:\n"
           "    -p or --path=<s>                    Path of the \"UltiHash\" Volume\n"
           "    -H or --agency-hostname=<s>         IP address or hostname of the \"UltiHash\" agency node.\n"
           "    -P or --agency-port=<i>             Port used to connect to the \"UltiHash\" agency node.\n"
           "    -C or --agency-connections=<i>      Number of connections to be used to communicate with the \"UltiHash\" agency node.\n"
           "    -h or --help                        Display this help message.\n"
           "\n");
}

void validate_options()
{
    auto& opt = uh::uhv::get_options();
    auto path = canonical(std::filesystem::path(opt.UHVpath));
    if(opt.agency_port < 0 or opt.agency_port > USHRT_MAX)
        THROW(uh::util::exception, "An invalid port number was specified.");
    if(opt.agency_connections < 0 )
        THROW(uh::util::exception, "An invalid number of connections was specified.");
}

int main(int argc, char *argv[])
{
    try
    {
        int ret = 0;
        struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

        auto& opt = uh::uhv::get_options();
        /* Default Values */
        opt.UHVpath = strdup("volume.uh");
        opt.agency_hostname = strdup("localhost");
        opt.agency_port = 21832;
        opt.agency_connections = 3;
        opt.show_help = false;

        /* Parse options */
        if (fuse_opt_parse(&args, &opt, option_spec, NULL) == -1)
            throw std::runtime_error("error: parsing failed");

        if (opt.show_help)
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
