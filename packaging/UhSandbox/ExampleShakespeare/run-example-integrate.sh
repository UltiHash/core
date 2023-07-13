#!/usr/bin/env bash

UH_SANDBOX=$HOME/UhSandbox
UH_CLI=$UH_SANDBOX/../bin/uh-cli
UH_CURRENT_EXAMPLE=$UH_SANDBOX/ExampleShakespeare

# Get some Shakespeare works from the Gutenberg Project Library
# wget -P $UH_CURRENT_EXAMPLE/test-input-dir https://www.gutenberg.org/files/1533/1533-0.txt
# wget -P $UH_CURRENT_EXAMPLE/test-input-dir https://www.gutenberg.org/files/27761/27761-0.txt
# wget -P $UH_CURRENT_EXAMPLE/test-input-dir https://www.gutenberg.org/cache/epub/1515/pg1515.txt
# wget -P $UH_CURRENT_EXAMPLE/test-input-dir https://www.gutenberg.org/cache/epub/1112/pg1112.txt
# wget -P $UH_CURRENT_EXAMPLE/test-input-dir https://www.gutenberg.org/cache/epub/100/pg100.txt


# Integrate some Shakespeare works in plain-text format to UltiHash
$UH_CLI -a localhost:21832 -i ./uh-shakespeare-volume.uh $UH_CURRENT_EXAMPLE/test-input-dir
