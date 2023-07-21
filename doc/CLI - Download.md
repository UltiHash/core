# Download

To download files or directories from UltiHash using the CLI, the command `uh-cli` must be invoked using the `-r` (or `--retrieve`) flag.

Example:

``` bash
$ uh-cli --retrieve <destination_uh_file_name>.uh --target <target_directory> --agency-node <host>:<port>
```

There are several options that can be used to modify the behaviour of uploads and downloads. They can be displayed at any time by invoking the `uh-cli` with the `-h` (or `--help`) flag:

```bash
$ uh-cli --help
```

The only option that is only relevant for downloading data from UltiHash is the `--target` directory:

```
  -T [ --target ] arg                   destination of the target directory for
                                        --retrieve(-r) operation [optional]
```