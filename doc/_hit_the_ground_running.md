## 

## Install UltiHash

Visit the [UltiHash closed-beta](https://www.ultihash.io/closed-beta) webpage and download the zipped installation package by clicking the button [Install](www.example.com).

Fire up a terminal and navigate to the `packaging` directory inside the folder you just unzipped:

```bash
$ cd setup-hardware/packaging
```

Install ansible and give it rights to become superuser:

```bash
sudo apt install ansible
ansible-playbook --ask-become-pass setup.yml
```

At this point, the executable `uh-cli` should be available.

## Getting Started - Hit the ground running / Test drive

This section describes how to start UltiHash in a single machine. Each component is started in its own terminal or run as background processes.

### 0. Navigate to the Sandbox

We will use this sandbox to store all data related to the examples, make sure you have permissions to write to that location.

```bash
mkdir -p $HOME/UhSandbox
cd $HOME/UhSandbox
```

### 1. Start the UltiHash _Data Node_
    
For convenience a bash script to do so is already provided. Launching it should start the UltiHash data node:
    
```
start-data-node.sh
```

Make sure that you read the script and modify the necessary environment variables before running it.
If the script is not runnable, you can make it runnable by running `chmod 777 start-data-node.sh`.

    
### 2. Run an example - Integrate

Open a new terminal.

A handful of Shakespeare works from the Gutenberg Project Library are provided in the directory ExampleShakespeare inside the sandbox.
A script with the command necessary to integrate the directory to UltiHash is provided for convenience:

```
run-example-integrate.sh
```

### 3. Run an example - Retrieve

The command to recover the UltiHashed Shakespeare works is very similar to the one used for integrating them. It is found in a runnable script as well: 

```
run-example-retrieve.sh
```

### 4. Make sure that the contents retrieved are the same as those in the original dataset 

To quickly compare the contents of two folders in Linux, we can use the `diff` utility

```
$ diff -rq test-output-dir test-input-dir
```

If the above command produces no output, the contents are the same.

### 5. Measure the space saved.

In Linux, the utility `du` offers a straightforward way to measure the space consumed by a directory.
To make a fair comparison, we need to measure the following:

 - the size of the original dataset,

```
$ du -h ./ExampleShakespeare/test-input-dir | tail -1
25M  ./ExampleShakespeare/test-input-dir
```

 - the size that UltiHash uses to save it after integration,

```
$ du -h ./ultihash-data/ | tail -1
6.2M ./ultihash-data/
```

 - the size of the recompilation file which, although small, needs to be taken into account.

```
$ ls -lh ./ExampleShakespeare/uh-shakespeare-volume.uh
2.7K ./ExampleShakespeare/uh-shakespeare-volume.uh
```

We see that the original data weights 25 MB, the ultihashed data around 25% of it and the recompilation fiie a negligible 2.7K. This means that, by storing this particular data set in UltiHash, we are saving 75% of space.