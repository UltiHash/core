# Upload

To upload files or directories to UltiHash using the CLI, the command `uh-cli` must be invoked using the `-i` (or `--integrate`) flag.

Example:

``` bash
$ uh-cli --integrate <destination_uh_file_name>.uh <origin_directory> --agency-node <host>:<port>
```

There are several options that can be used to modify the behaviour of the upload, and can be displayed at any time by invoking the `uh-cli` with the `-h` (or `--help`) flag:

```bash
$ uh-cli --help
```

In the following, the most relevant options for uploading data to UltiHash are described.


## Excluding directories

```
  -E [ --exclude ] arg                  exclude directories from being uploaded
                                        [optional]
```


## Chunking Options - DEPRECATED

```
  --chunking-strategy arg (=FixedSize)  Strategy to use for spliting files into
                                        chunks
  --chunk-size arg (=4194304)           Size in bytes of the chunks, in case of
                                        a fixed size chunking strategy
  --gear-max-size arg                   maximum chunk size for Gear CDC
  --gear-avg-size arg                   average chunk size for Gear CDC
  --fastcdc-min-size arg                minimum chunk size for FastCDC
  --fastcdc-max-size arg                maximum chunk size for FastCDC
  --fastcdc-normal-size arg             normal chunk size for FastCDC
  --modcdc-min-size arg                 minimum chunk size for ModCDC
  --modcdc-max-size arg                 maximum chunk size for ModCDC
  --modcdc-normal-size arg              normal chunk size for ModCDC
```

## Example Usage

```bash 
$ uh-cli --integrate <destination_uh_file_name>.uh <origin_directory> --agency-node <host>:<port>
```
