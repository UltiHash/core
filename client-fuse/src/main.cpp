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

static struct options
{
    const char *UHVpath;
    const char *agency_connection;
    const char *mountpoint;
    int show_help;
} options;

#define OPTION(t, p)                           \
    { t, offsetof(struct options, p), 1 }
static const struct fuse_opt option_spec [] =
    {
        OPTION("--path=%s", UHVpath),
        OPTION("-p=%s", UHVpath),
        OPTION("--agency=%s", agency_connection),
        OPTION("-a=%s", agency_connection),
        OPTION("--mount=%s", mountpoint),
        OPTION("-m=%s", mountpoint),
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

int uh_read (const char *, char *, size_t, off_t, struct fuse_file_info *)
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
           "    --name=<s>          Name of the \"hello\" file\n"
           "                        (default: \"hello\")\n"
           "    --contents=<s>      Contents \"hello\" file\n"
           "                        (default \"Hello, World!\\n\")\n"
           "\n");
}

int main(int argc, char *argv[])
{
    try
    {
        int ret;

        struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

        /* Parse options */
        if (fuse_opt_parse(&args, &options, option_spec, NULL) == -1)
            return 1;

        if (options.show_help)
        {
            show_help(argv[0]);
            assert(fuse_opt_add_arg(&args, "--help") == 0);
            args.argv[0][0] = '\0';
        }

        ret = fuse_main(args.argc, args.argv, &uh_operations, NULL);
        fuse_opt_free_args(&args);
        return ret;
    }
    catch (const std::exception& exc)
    {
        return 0;
    }
}