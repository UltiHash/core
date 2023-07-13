#!/usr/bin/env bash

UH_SANDBOX=$HOME/UhSandbox
UH_CLI=$UH_SANDBOX/../bin/uh-cli
UH_CURRENT_EXAMPLE=$UH_SANDBOX/ExampleShakespeare

# Create target directory before retrieving
mkdir -p $UH_CURRENT_EXAMPLE/test-output-dir

# Retrieve from UltiHash volume
$UH_CLI -r ./uh-shakespeare-volume.uh -T $UH_CURRENT_EXAMPLE/test-output-dir -a localhost:21832

#Compare to original contents
diff -rq test-output-dir test-input-dir
