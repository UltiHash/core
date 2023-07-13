# CLI options

The CLI offers several options to control data upload and download. The options that are relevant for both upload and download are briefly described in what follows.

## Information display

```
  -h [ --help ] [=arg(=1)]              print help screen
  -v [ --version ] [=arg(=1)]           output program version
  -V [ --verbose ]                      shows details about the results of 
                                        running the command [optional]
```


## Configuration file

All options defaults can be stored in a configuration file using the syntax `key = value`, with each key-value pair written in a separate line.

```
  -c [ --config ] arg                   load configuration file from the given location
```


## Using multiple threads

```bash
  -j [ --jobs ] arg                     size of the worker threads when 
                                        uploading and downloading
```

## Network parameters

```
  -a [ --agency-node ] arg              <HOSTNAME[:PORT]> of agency node to 
                                        connect to (port defaults to 8565)
  -P [ --pool-size ] arg                pool size of connections to the agency 
                                        node [optional]
```