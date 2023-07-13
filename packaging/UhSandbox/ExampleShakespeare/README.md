# Integrate repeated plain text files

The purpose of this example is to show how to interact with UltiHash CLI.

The most trivial example is to integrate repeated files.
The folder `test-input-dir` of this example contains 4 copies of 5 different Shakespeare works.
On integration, we should expect the volume of the integrated data to be 1/4 of its original size (i.e., 25%)

There are two bash scripts with the necessary commands to run the example:

 - `$ run-example-integrate.sh` to integrate
 - `$ run-example-retrieve.sh` to retrieve

Please read the contents of the scripts before executing them.
