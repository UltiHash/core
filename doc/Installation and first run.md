# Installation and first run

To run the UltiHash data node server, you need at least 40GB of free storage space. Please make sure that your system can provide them prior to installation.

## Running the UltiHash Data Node server and CLI

To use our Docker-based release, please make sure to have a fully working installation of Docker configured on your system before proceeding.
To make sure you have the latest Docker images of our data node server and CLI, please make sure to run the following commands:
```
docker pull ultihash/uh-data-node:latest
docker pull ultihash/uh-cli:latest
```
### Starting the Data Node server
You can easily start the UltiHash Data Node server by running:
```
docker run -p 21832:21832 --name uh-data-node ultihash/uh-data-node
```

### Running the CLI through Docker
You can also run the CLI through docker. 
We recommend that you make your current working directory accessible for the Docker container running the CLI.
That way, you can easily integrate and retrieve test data sets on your machine.

#### Integrating Data
To integrate test data from your host machine, you can run the following command:
```
docker run -it -v $(pwd):/data --link uh-data-node ultihash/uh-cli uh-cli -a uh-data-node:21832 --integrate /data/test.uh /data/[TEST_DATA_TO_INTEGRATE] 
```
Make sure to replace the placeholder `[TEST_DATA_TO_INTEGRATE]` with the path to the test data you want to integrate, e.g. `/data/IMAGE-ASTRONOMY-glimpse`. The command line output of running the above command should look like this:
```
max@aquinas:~/Desktop$ docker run -it -v $(pwd):/data --link uh-data-node ultihash/uh-cli uh-cli -a uh-data-node:21832 -i /data/test.uh /data/IMAGE-ASTRONOMY-glimpse/
INPUT: "/data/IMAGE-ASTRONOMY-glimpse" 
OUTPUT: "/data/test.uh"
integration time: 1.50755
space saving: 0.481898
encoding speed: 569.374 Mb/s
```
The `.uh` file you get from each integration process contains a list of keys identifying the chunks of data the input has been split up into.

#### Retrieving Data
To retrieve test data from the Data Node using a `.uh` file, you can run the following command:
```
docker run -it -v $(pwd):/data --link uh-data-node ultihash/uh-cli uh-cli -a uh-data-node:21832 --retrieve /data/test.uh --target /data/output
```
The command line output should look like this:
```
max@aquinas:~/Desktop$ docker run -it -v $(pwd):/data --link uh-data-node ultihash/uh-cli uh-cli -a uh-data-node:21832 --retrieve /data/test.uh --target /data/output
INPUT: "/data/test.uh" 
OUTPUT: "/data/output"
retrieval time: 0.748069
decoding speed: 1147.44 Mb/s
```


## Using the UltiHash SDK

If you want to use our C SDK to interface with our Data Node server, please download the SDK package from [ultihash.io/beta](https://www.ultihash.io/beta)

Extract the package:

```
$ unzip UltiHash-x86_64-ubuntu-22.04-20230803-Release.zip
```

Navigate inside the unpacked `UltiHash-x86_64-ubuntu-22.04-20230803-Release` directory:

```
$ cd UltiHash-x86_64-ubuntu-22.04-20230803-Release
```

## Run the setup script

To install the package on your computer please run:

```
$ source setup.sh
```

Alternatively to change the installation prefix you can edit the `setup.sh` script and set the `prefix` variable to another destination.

## Start the Data Node in the background
Please refer to the Docker-based instructions above.

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


