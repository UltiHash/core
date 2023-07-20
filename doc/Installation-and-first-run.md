# Installation and first run

Download the package from [ultihash.io/beta](https://www.ultihash.io/beta)

Extract the package:

```
$ tar -xf UltiHashReleasePackage.tar.bz2
```

Navigate inside the unpackted `UltiHashReleasePackage` directory:

```
$ cd UltiHashReleasePackage
```

## Run the setup script

To install the package on your computer please run:

```
$ source setup.sh
```

Alternatively to change the installation prefix you can edit the `setup.sh` script and set the `prefix` variable to another destination.

## Start the data node in the background

```
$ uh-data-node --data-dir $PWD/uh-data-dir > data-node.out 2> data-node.err &
```

## Compile and run a simple example using the UltiHash SDK

A sample C++ code that uses the UltiHash SDK is provided in this package under the name `simple_example.cpp`. You can compile and run the sample code by linking it to the UltiHash library as follows:

```
$ g++ -o simple_example simple_example.cpp -ludb

$ ./simple_example
```

You should expect to see the following output:

```
UDB 0.1.0

Effective Sizes: 664 661

Return code: 0 0

Received Key:
This_is_a_user_defined_key_1

Received Value:
The quick brown fox jumps over the lazy dog [...]

Received Key:
This_is_a_user_defined_key_2

Received Value:
Lorem Ipsum comes from a latin text [...]
```

## Integrate some directories to UltiHash using the CLI

```
$ uh-cli --integrate <some_name>.uh <some_directory> --agency-node localhost:21832
```


## Retrieve integrated directories from UltiHash using the CLI

```
$ uh-cli --retrieve <some_existing_uh_file>.uh --target <target_directory> --agency-node localhost:21832
```

## Terminate the data-node running in the background

$ kill $(ps -a | grep uh-data-node)
