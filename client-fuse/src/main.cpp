/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

  This program can be distributed under the terms of the GNU GPLv2.
  See the file COPYING.
*/

#define FUSE_USE_VERSION 31

#include <iostream>
#include <fuse.h>
#include <cstring>
#include <cassert>
#include <logging/logging_boost.h>

static struct options
{
    const char *UHVpath;
    const char *agency_connection;
    const char *mountpoint;
    bool show_help;
} options;

#define OPTION(t, p)                           \
    { t, offsetof(struct options, p), 1 }
static const struct fuse_opt option_spec[] =
    {
        OPTION("--path=%s", UHVpath),
        OPTION("-p=%s", UHVpath),
        OPTION("--agency=%s", agency_connection),
        OPTION("-a=%s", agency_connection),
        OPTION("--mount=%s", mountpoint),
        OPTION("-m=%s", mountpoint),
        OPTION("--help", show_help),
        OPTION("-h", show_help),
        OPTION("--help", show_help),
        FUSE_OPT_END
    };

int uh_getattr (const char *, struct stat *)
{
    return 0;
}

void *uh_init (struct fuse_conn_info *conn)
{
    return NULL;
}

int uh_readdir (const char *, void *, fuse_fill_dir_t, off_t, struct fuse_file_info *)
{
    return 0;
}

int uh_open (const char *, struct fuse_file_info *)
{
    return 0;
}

int uh_read (const char *, char *, size_t, off_t,
		     struct fuse_file_info *)
{
    return 0;
}

static const struct fuse_operations uh_operations =
    {
    .getattr        = uh_getattr,
    .open           = uh_open,
    .read           = uh_read,
    .readdir        = uh_readdir,
    .init           = uh_init,
    };

static void show_help(const char *progname)
{
    printf("usage: %s [options] <mountpoint>\n\n", progname);
    printf("File-system specific options:\n"
           "    -p or --path=<s>          Path of the \"UltiHash\" Volume\n"
           "    -a or --agency=<s>        Contents \"hello\" file\n"
           "    -m or --mount=<s>         Name of the \"hello\" file\n"
           "\n");
}

int main(int argc, char *argv[])
{

    try
    {
        struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

        options.UHVpath = strdup("/home/ankit/Downloads/");
        options.agency_connection = strdup("localhost:8083");
        options.mountpoint = strdup("/home/ankit/Downloads/");
        options.show_help = true;

        /* Parse options */
        if (fuse_opt_parse(&args, &options, option_spec, NULL) == -1);
            throw std::runtime_error("error: parsing failed");
        if (options.show_help)
        {
            show_help(argv[0]);
            assert(fuse_opt_add_arg(&args, "--help") == 0);
            args.argv[0][0] = '\0';
        }

        if (fuse_main(args.argc, args.argv, &uh_operations, NULL) == -1)
            throw std::runtime_error("error: failed to start fuse service");

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
    return EXIT_SUCCESS;
}

// ---------------------------------------------------------------------
